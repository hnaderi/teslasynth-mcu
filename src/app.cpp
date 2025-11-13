#include "app.hpp"
#include "configuration/synth.hpp"
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

Config config;
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

void render(void *pvParams) {
  Sequencer<> seq(config, notes, track);
  constexpr TickType_t loopTime = pdMS_TO_TICKS(10);
  constexpr size_t BUFFER_SIZE = 20;

  int64_t processed = esp_timer_get_time();
  Pulse buffer[BUFFER_SIZE];

  while (true) {
    vTaskDelay(loopTime);

    // TODO what if lags behind?
    xSemaphoreTake(xNotesMutex, portMAX_DELAY);
    int64_t now = esp_timer_get_time();
    size_t i = 0;
    for (; processed < now && i < BUFFER_SIZE; i++) {
      auto left = Duration::micros(now - processed);
      buffer[i] = seq.sample(left);
      processed += buffer[i].length().micros();
    }
    xSemaphoreGive(xNotesMutex);
    if (i > 0) {
      pulse_write(buffer, i);
    }
  }
}

void play(StreamBufferHandle_t sbuf) {
  config = load_config();
  rmt_driver();
  xQueue = xQueueCreate(8, sizeof(MidiChannelMessage));
  if (xQueue == NULL)
    return;
  xNotesMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(parser, "Parser", 8 * 1024, sbuf, 2, NULL, 1);
  xTaskCreatePinnedToCore(synth, "Synth", 8 * 1024, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(render, "Render", 8 * 1024, NULL, 2, NULL, 1);
}
