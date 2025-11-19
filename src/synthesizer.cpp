#include "configuration/synth.hpp"
#include "core.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "instruments.hpp"
#include "midi_core.hpp"
#include "midi_parser.hpp"
#include "midi_synth.hpp"
#include "notes.hpp"
#include "output/rmt_driver.h"
#include "portmacro.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

Notes notes;
TrackState track;
SemaphoreHandle_t xNotesMutex;

static const char *TAG = "SYNTH";

void synth(void *pvParams) {
  MidiChannelMessage msg;
  SynthChannel ch(get_config(), notes, track, instruments, 20);
  MidiParser parser([&](const MidiChannelMessage msg) {
    auto now = Duration::micros(esp_timer_get_time());
    ESP_LOGD(TAG, "Received: %s at %s", std::string(msg).c_str(),
             std::string(now).c_str());
    ch.handle(msg, now);
  });
  StreamBufferHandle_t sbuf = static_cast<StreamBufferHandle_t>(pvParams);
  uint8_t buffer[256];
  while (true) {
    size_t read =
        xStreamBufferReceive(sbuf, buffer, sizeof(buffer), portMAX_DELAY);

    if (read) {
      xSemaphoreTake(xNotesMutex, portMAX_DELAY);
      parser.feed(buffer, read);
      xSemaphoreGive(xNotesMutex);
    }
  }
}

void render(void *) {
  constexpr TickType_t loopTime = pdMS_TO_TICKS(10);
  TickType_t lastTime = xTaskGetTickCount();
  constexpr size_t BUFFER_SIZE = 64;

  Sequencer<> seq(get_config(), notes, track);
  int64_t processed = esp_timer_get_time();
  Pulse buffer[BUFFER_SIZE];

  while (true) {
    vTaskDelayUntil(&lastTime, loopTime);

    xSemaphoreTake(xNotesMutex, portMAX_DELAY);
    size_t i = 0;
    int64_t now = esp_timer_get_time();
    for (; processed < now && i < BUFFER_SIZE; i++) {
      auto left = Duration::micros(now - processed);
      buffer[i] = seq.sample(left);
      processed += buffer[i].length().micros();
    }
    xSemaphoreGive(xNotesMutex);
    pulse_write(buffer, i);
  }
}

void init_synth(StreamBufferHandle_t sbuf) {
  auto config = load_config();
  notes = Notes(config);
  rmt_driver();
  xNotesMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(synth, "Synth", 8 * 1024, sbuf, 2, NULL, 1);
  xTaskCreatePinnedToCore(render, "Render", 8 * 1024, NULL, 2, NULL, 1);
}
