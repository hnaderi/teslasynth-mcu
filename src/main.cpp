#include "application.hpp"
#include "configuration/synth.hpp"
#include "esp_event.h"
#include "teslasynth.hpp"

using namespace teslasynth::app;

extern "C" void app_main(void) {
  devices::storage::init();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  Application app(configuration::read());

#ifndef CONFIG_TESLASYNTH_GUI_NONE
  gui::init();
#endif
  cli::init(app.ui());
  devices::rmt::init();
  auto sbuf = devices::ble_midi::init();
  synth::init(sbuf, app.playback());
  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
