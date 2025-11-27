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
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

ESP_EVENT_DEFINE_BASE(EVENT_SYNTHESIZER_BASE);

namespace teslasynth::app::synth {
using namespace teslasynth::midisynth;
using namespace teslasynth::app::configuration;

Notes notes;
TrackState track;
SemaphoreHandle_t xNotesMutex;

static const char *TAG = "SYNTH";

static void synth(void *pvParams) {
  MidiChannelMessage msg;
  SynthChannel channel(get_config(), notes, track, instruments,
                       instruments_size);
  MidiParser parser([&](const MidiChannelMessage msg) {
    auto now = Duration::micros(esp_timer_get_time());
    ESP_LOGD(TAG, "Received: %s at %s", std::string(msg).c_str(),
             std::string(now).c_str());
    channel.handle(msg, now);
  });
  StreamBufferHandle_t sbuf = static_cast<StreamBufferHandle_t>(pvParams);
  uint8_t buffer[256];
  bool playing = false, was_playing = playing;
  while (true) {
    size_t read =
        xStreamBufferReceive(sbuf, buffer, sizeof(buffer), portMAX_DELAY);

    if (read) {
      xSemaphoreTake(xNotesMutex, portMAX_DELAY);
      parser.feed(buffer, read);
      playing = track.is_playing();
      xSemaphoreGive(xNotesMutex);

      if (playing != was_playing) {
        if (playing) {
          ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(
              EVENT_SYNTHESIZER_BASE, SYNTHESIZER_PLAYING, NULL, 0, 0));
        } else {
          ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(
              EVENT_SYNTHESIZER_BASE, SYNTHESIZER_STOPPED, NULL, 0, 0));
        }
        was_playing = playing;
      }
    }
  }
}

static void render(void *) {
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
    devices::rmt::pulse_write(buffer, i);
  }
}

void init(StreamBufferHandle_t sbuf) {
  auto config = load_config();
  notes = Notes(config);
  xNotesMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(synth, "Synth", 8 * 1024, sbuf, 10, NULL, 1);
  xTaskCreatePinnedToCore(render, "Render", 8 * 1024, NULL, 10, NULL, 1);
}

} // namespace teslasynth::app::synth
