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
TrackState track;
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
  SynthChannel ch(config, notes, track);
  MidiChannelMessage msg;
  while (true) {
    BaseType_t status = xQueueReceive(xQueue, &msg, xTicksToWait);

    // TODO duration overflows now
    Duration now = Duration::micros(esp_timer_get_time());

    if (status) {
      ESP_LOGI(TAG, "Received: %s at %s", std::string(msg).c_str(),
               std::string(now).c_str());
      xSemaphoreTake(xNotesMutex, portMAX_DELAY);
      ch.handle(msg, now);
      xSemaphoreGive(xNotesMutex);
    }
  }
}

void test_tune(void *pvParams) {
  Pulse pulses[] = {
      {250_us, 1_ms}, {0_us, 1_ms},   {400_us, 1_ms},
      {100_us, 1_ms}, {100_us, 6_ms},
  };
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(10));
    pulse_write(pulses, 5);
  }
}

void render(void *pvParams) {
  Sequencer<> seq(config, notes, track);
  constexpr TickType_t prefillTime = pdMS_TO_TICKS(10),
                       loopTime = pdMS_TO_TICKS(5);
  constexpr size_t BUFFER_SIZE = 20;
  vTaskDelay(prefillTime);

  int64_t processed = esp_timer_get_time();
  Pulse buffer[BUFFER_SIZE];

  while (true) {
    vTaskDelay(loopTime);
    xSemaphoreTake(xNotesMutex, portMAX_DELAY);

    int64_t now = esp_timer_get_time();
    for (size_t i = 0; processed < now; i++) {
      auto left = Duration::micros(now - processed);
      buffer[i] = seq.sample(left);
      processed += buffer[i].period.micros();
      if (i == BUFFER_SIZE || processed >= now) {
        pulse_write(buffer, i + 1);
        i = 0;
      }
    }
    xSemaphoreGive(xNotesMutex);
  }
}

void debug_log(void *) {
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(100));
    xSemaphoreTake(xNotesMutex, portMAX_DELAY);
    auto now = *(Duration::micros(esp_timer_get_time()) - track.started_time());
    ESP_LOGI(TAG, "Started: %s, recv: %s, play: %s, now: %s",
             std::string(track.started_time()).c_str(),
             std::string(track.received_time()).c_str(),
             std::string(track.played_time()).c_str(),
             std::string(now).c_str());
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
  // xTaskCreatePinnedToCore(debug_log, "Debug", 8 * 1024, NULL, 1, NULL, 1);
}
