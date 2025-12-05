#pragma once

#include "esp_event.h"
#include "freertos/idf_additions.h"
#include "midi_synth.hpp"
#include "sdkconfig.h"
#include "synthesizer_events.hpp"

namespace teslasynth::app {
using namespace midisynth;

namespace {
typedef Teslasynth<CONFIG_TESLASYNTH_OUTPUT_COUNT> TSYNTH;
typedef Configuration<CONFIG_TESLASYNTH_OUTPUT_COUNT> AppConfig;

void on_track_play(bool playing) {
  if (playing) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(
        EVENT_SYNTHESIZER_BASE, SYNTHESIZER_PLAYING, NULL, 0, 0));
  } else {
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_post(
        EVENT_SYNTHESIZER_BASE, SYNTHESIZER_STOPPED, NULL, 0, 0));
  }
}
}; // namespace

class PlaybackHandle {
  TSYNTH *impl;
  SemaphoreHandle_t lock;

public:
  PlaybackHandle() {}
  PlaybackHandle(TSYNTH *impl, SemaphoreHandle_t lock)
      : impl(impl), lock(lock) {}

  inline void acquire() { xSemaphoreTake(lock, portMAX_DELAY); }
  inline void release() { xSemaphoreGive(lock); }

  inline void handle(MidiChannelMessage msg, Duration time) {
    impl->handle(msg, time);
  }
  template <size_t BUFSIZE>
  inline void
  sample_all(Duration16 max,
             PulseBuffer<CONFIG_TESLASYNTH_OUTPUT_COUNT, BUFSIZE> &output) {
    impl->sample_all(max, output);
  };
};

class UIHandle {
  TSYNTH *impl;
  SemaphoreHandle_t write_lock, read_lock;

public:
  UIHandle() {}
  UIHandle(TSYNTH *impl, SemaphoreHandle_t write, SemaphoreHandle_t read)
      : impl(impl), write_lock(write), read_lock(read) {}

  inline constexpr auto &config_read() const {
    xSemaphoreTake(read_lock, portMAX_DELAY);
    auto &res = impl->configuration();
    xSemaphoreGive(read_lock);
    return res;
  }

  inline constexpr void config_set(const AppConfig &config,
                                   bool reload = false) {
    xSemaphoreTake(read_lock, portMAX_DELAY);

    xSemaphoreTake(write_lock, portMAX_DELAY);
    impl->configuration() = config;
    if (reload)
      impl->reload_config();
    xSemaphoreGive(write_lock);

    xSemaphoreGive(read_lock);
    ESP_ERROR_CHECK(esp_event_post(EVENT_SYNTHESIZER_BASE,
                                   SYNTHESIZER_CONFIG_UPDATED, NULL, 0,
                                   portMAX_DELAY));
  }

  inline constexpr void playback_off() {
    xSemaphoreTake(write_lock, portMAX_DELAY);
    impl->off();
    xSemaphoreGive(write_lock);
  }
};

class Application {
  TSYNTH impl;
  SemaphoreHandle_t write_lock, read_lock;

public:
  Application(const AppConfig &config)
      : impl(config, on_track_play), write_lock(xSemaphoreCreateMutex()),
        read_lock(xSemaphoreCreateMutex()) {}
  PlaybackHandle playback() { return PlaybackHandle(&impl, write_lock); }
  UIHandle ui() { return UIHandle(&impl, write_lock, read_lock); }
};
}; // namespace teslasynth::app
