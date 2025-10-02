#include "ble-midi.h"
#include "example.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <synth.hpp>

// Override the weak callback
void midi_rx_callback(const uint8_t *data, uint16_t len) {
  printf("Got MIDI: ");
  for (int i = 0; i < len; i++)
    printf("%02X ", data[i]);
  printf("\n");

  // TODO: parse MIDI here (e.g. Note On/Off) and trigger RMT
}

extern "C" void app_main(void) {
  SynthChannel channel = SynthChannel(Config{});
  channel.on_note_on(1, 1, 1_ms);
  example();
  ble_midi_receiver_init();
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
