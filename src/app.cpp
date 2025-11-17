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
const TickType_t xTicksToWait = pdMS_TO_TICKS(10);
Notes notes;
TrackState track;
SemaphoreHandle_t xNotesMutex;

static const char *TAG = "APP";

  SynthChannel ch(config, notes, track);
void synth(void *pvParams) {
  MidiChannelMessage msg;
  QueueHandle_t xQueue = static_cast<QueueHandle_t>(pvParams);
  while (true) {
    BaseType_t status = xQueueReceive(xQueue, &msg, xTicksToWait);

    Duration now = Duration::micros(esp_timer_get_time());

    if (status) {
      ESP_LOGD(TAG, "Received: %s at %s", std::string(msg).c_str(),
               std::string(now).c_str());
      xSemaphoreTake(xNotesMutex, portMAX_DELAY);
      ch.handle(msg, now);
      xSemaphoreGive(xNotesMutex);
    }
  }
}

void render(void *) {
  Sequencer<> seq(config, notes, track);
  constexpr TickType_t loopTime = pdMS_TO_TICKS(5);
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

void play(QueueHandle_t channelMessages) {
  config = load_config();
  rmt_driver();
  xNotesMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(synth, "Synth", 8 * 1024, channelMessages, 2, NULL,
                          1);
  xTaskCreatePinnedToCore(render, "Render", 8 * 1024, NULL, 2, NULL, 1);
}
