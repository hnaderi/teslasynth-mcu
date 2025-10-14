#include "example.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <synth.hpp>

extern "C" void app_main(void) {
  Notes notes;
  Config config;
  SynthChannel channel = SynthChannel(config, notes);
  channel.on_note_on(1, 1, 1_ms);
  example();
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
