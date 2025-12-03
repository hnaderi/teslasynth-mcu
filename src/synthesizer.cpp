#include "configuration/synth.hpp"
#include "core.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "instruments.hpp"
#include "midi_core.hpp"
#include "midi_parser.hpp"
#include "midi_synth.hpp"
#include "notes.hpp"
#include "output/rmt_driver.hpp"
#include "portmacro.h"
#include "synthesizer_events.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <stddef.h>
#include <string>

ESP_EVENT_DEFINE_BASE(EVENT_SYNTHESIZER_BASE);

namespace teslasynth::app::synth {
using namespace teslasynth::midisynth;
using namespace teslasynth::app::configuration;

void on_track_play(bool playing) {
  if (playing) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(
        EVENT_SYNTHESIZER_BASE, SYNTHESIZER_PLAYING, NULL, 0, 0));
  } else {
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(
        EVENT_SYNTHESIZER_BASE, SYNTHESIZER_STOPPED, NULL, 0, 0));
  }
}

Configuration<CONFIG_TESLASYNTH_OUTPUT_COUNT> config;
Teslasynth<CONFIG_TESLASYNTH_OUTPUT_COUNT> tsynth(config, on_track_play);
SemaphoreHandle_t xNotesMutex;

static const char *TAG = "SYNTH";

static void synth(void *pvParams) {
  MidiChannelMessage msg;
  MidiParser parser([&](const MidiChannelMessage msg) {
    auto now = Duration::micros(esp_timer_get_time());
#if CONFIG_TESLASYNTH_DEBUG
    ESP_LOGI(TAG, "Received: %s at %s", std::string(msg).c_str(),
             std::string(now).c_str());
#endif
    tsynth.handle(msg, now);
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

static void render(void *) {
  constexpr TickType_t loopTime = pdMS_TO_TICKS(10);
  TickType_t lastTime = xTaskGetTickCount();

  int64_t processed = esp_timer_get_time();
  PulseBuffer<CONFIG_TESLASYNTH_OUTPUT_COUNT, 64> buffer;

  while (true) {
    vTaskDelayUntil(&lastTime, loopTime);

    xSemaphoreTake(xNotesMutex, portMAX_DELAY);
    auto now = esp_timer_get_time();
    auto left = now - processed;
    uint16_t ll = static_cast<uint16_t>(
        std::min<int64_t>(left, std::numeric_limits<uint16_t>::max()));
    tsynth.sample_all(Duration16::micros(ll), buffer);
    xSemaphoreGive(xNotesMutex);

    for (uint8_t ch = 0; ch < CONFIG_TESLASYNTH_OUTPUT_COUNT; ch++) {
      devices::rmt::pulse_write(&buffer.data(ch), buffer.data_size(ch), ch);
    }

    uint32_t took = esp_timer_get_time() - processed;
    processed = now;

    static size_t counter = 0;
    static uint32_t min_i, max_i, total = 0;
    if (counter == 0) {
      min_i = max_i = took;
    } else {
      min_i = std::min(took, min_i);
      max_i = std::max(took, max_i);
    }
    total += took;

#if CONFIG_TESLASYNTH_DEBUG
    if (counter++ % 100 == 0) {
      ESP_LOGI(TAG,
               "Render stats, min: %u, max: %u, total: %u, avg: %u, ctr: %u",
               min_i, max_i, total, total / counter, counter);
    }
#endif
  }
}

void init(StreamBufferHandle_t sbuf) {
  xNotesMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(synth, "Synth", 8 * 1024, sbuf, 10, NULL, 1);
  xTaskCreatePinnedToCore(render, "Render", 8 * 1024, NULL, 10, NULL, 1);
}

} // namespace teslasynth::app::synth
