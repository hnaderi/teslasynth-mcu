#include "app.hpp"
#include <input/ble_midi.hpp>

extern "C" void app_main(void) {
  auto sbuf = ble_begin("Teslasynth");
  play(sbuf);
  // rmt_driver();
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
