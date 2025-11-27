#include "configuration/synth.hpp"
#include "esp_event.h"
#include "teslasynth.hpp"

using namespace teslasynth::app;

extern "C" void app_main(void) {
  devices::storage::init();
  configuration::load_config();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifndef CONFIG_TESLASYNTH_GUI_NONE
  gui::init();
#endif
  cli::init();
  devices::rmt::init();
  auto sbuf = devices::ble_midi::init();
  synth::init(sbuf);
  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
