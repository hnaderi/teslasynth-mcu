#include "app.hpp"
#include "core.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "midi_core.hpp"
#include "midi_parser.hpp"
#include "midi_synth.hpp"
#include "notes.hpp"
#include "output/rmt_driver.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

const Config config{
    .max_on_time = 500_us,
};
QueueHandle_t xQueue;
const TickType_t xTicksToWait = pdMS_TO_TICKS(10);
Notes notes;
SemaphoreHandle_t xNotesMutex;

static const char *TAG = "APP";

void cbk(const MidiChannelMessage &msg) {
  if (xQueueSendToBack(xQueue, &msg, xTicksToWait) != pdPASS) {
    ESP_LOGE(TAG, "Couldn't enqueue channel message!");
  }
}

void parser(void *pvParams) {
  StreamBufferHandle_t sbuf = static_cast<StreamBufferHandle_t>(pvParams);
  MidiParser parser(cbk);
  uint8_t buffer[64];
  size_t read = 0;

  while (true) {
    read = xStreamBufferReceive(sbuf, buffer, sizeof(buffer), xTicksToWait);
    parser.feed(buffer, read);
  }
}

void synth(void *pvParams) {
  SynthChannel ch(config, notes);
  MidiChannelMessage msg;
  int64_t time = esp_timer_get_time();
  while (true) {
    BaseType_t status = xQueueReceive(xQueue, &msg, xTicksToWait);
    int64_t now = esp_timer_get_time();
    Duration delta = Duration::micros(now - time);

    if (status) {
      ESP_LOGI(TAG, "Received: %s", std::string(msg).c_str());
      xSemaphoreTake(xNotesMutex, portMAX_DELAY);
      ch.handle(msg, delta);
      xSemaphoreGive(xNotesMutex);
    }
  }
}

void render(void *pvParams) {
  Sequencer<> seq(config, notes);
  constexpr TickType_t prefillTime = pdMS_TO_TICKS(100),
                       loopTime = pdMS_TO_TICKS(10);
  constexpr size_t BUFFER_SIZE = 20;
  vTaskDelay(prefillTime);

  int64_t processed = esp_timer_get_time();
  NotePulse buffer[BUFFER_SIZE];

  while (true) {
    vTaskDelay(loopTime);
    int64_t now = esp_timer_get_time();

    xSemaphoreTake(xNotesMutex, portMAX_DELAY);
    for (size_t i = 0; processed < now; i++) {
      auto left = Duration::micros(now - processed);
      buffer[i] = seq.sample(left);
      processed += buffer[i].period.micros<uint32_t>();
      ESP_LOGI(TAG, "PULSE: %s, left: %u", std::string(buffer[i]).c_str(),
               left.micros<uint32_t>());
      if (i == BUFFER_SIZE || processed >= now) {
        ESP_LOGI(TAG, "Writting rmt buffer, %u p:%u", i + 1, processed);
        pulse_write(buffer, i + 1);
        i = 0;
      }
    }
    xSemaphoreGive(xNotesMutex);
  }
}

void play(StreamBufferHandle_t sbuf) {
  rmt_driver();
  xQueue = xQueueCreate(8, sizeof(MidiChannelMessage));
  if (xQueue == NULL)
    return;
  xNotesMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(parser, "Parser", 8 * 1024, sbuf, 1, NULL, 1);
  xTaskCreatePinnedToCore(synth, "Synth", 8 * 1024, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(render, "Render", 8 * 1024, NULL, 1, NULL, 1);
}
