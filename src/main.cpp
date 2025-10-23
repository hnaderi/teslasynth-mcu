#include "app.hpp"
#include <input/ble_midi.hpp>

extern "C" void app_main(void) {
  auto sbuf = ble_begin("Teslasynth");
  play(sbuf);
  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
