#include "application.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "midi_core.hpp"
#include "midi_parser.hpp"
#include "midi_synth.hpp"
#include "output/rmt_driver.hpp"
#include "portmacro.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stddef.h>

namespace teslasynth::app::synth {
using namespace teslasynth::midisynth;

static const char *TAG = "SYNTH";

static PlaybackHandle playback;
static StreamBufferHandle_t stream;

static void input(void *) {
  MidiChannelMessage msg;
  MidiParser parser([&](const MidiChannelMessage msg) {
    auto now = Duration64::micros(esp_timer_get_time());
#if CONFIG_TESLASYNTH_DEBUG
    ESP_LOGI(TAG, "Received: %s at %s", std::string(msg).c_str(),
             std::string(now).c_str());
#endif
    playback.handle(msg, now);
  });
  uint8_t buffer[256];
  while (true) {
    size_t read =
        xStreamBufferReceive(stream, buffer, sizeof(buffer), portMAX_DELAY);

    if (read) {
      playback.acquire();
      parser.feed(buffer, read);
      playback.release();
    }
  }
}

static void output(void *pvParams) {
  constexpr TickType_t loopTime = pdMS_TO_TICKS(10);
  TickType_t lastTime = xTaskGetTickCount();

  int64_t processed = esp_timer_get_time();
  PulseBuffer<CONFIG_TESLASYNTH_OUTPUT_COUNT, 64> buffer;

  while (true) {
    vTaskDelayUntil(&lastTime, loopTime);

    playback.acquire();
    auto now = esp_timer_get_time();
    auto left = now - processed;
    auto budget = Duration16::micros(static_cast<uint16_t>(
        std::min<int64_t>(left, std::numeric_limits<uint16_t>::max())));
    playback.sample_all(budget, buffer);
    playback.release();

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

void init(StreamBufferHandle_t sbuf, PlaybackHandle handle) {
  ESP_LOGD(TAG, "init");
  playback = handle;
  stream = sbuf;
  xTaskCreatePinnedToCore(input, "Input", 8 * 1024, nullptr, 10, nullptr, 1);
  xTaskCreatePinnedToCore(output, "Output", 8 * 1024, nullptr, 10, nullptr, 1);
}

} // namespace teslasynth::app::synth
