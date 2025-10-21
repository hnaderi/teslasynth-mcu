#include "app.hpp"
#include "core.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "midi_core.hpp"
#include "midi_parser.hpp"
#include "midi_synth.hpp"
#include "synth.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

QueueHandle_t xQueue;
const TickType_t xTicksToWait = pdMS_TO_TICKS(10);
Notes notes;

static const char *TAG = "APP";

void cbk(const MidiChannelMessage &msg) {
  ESP_LOGI(TAG, "Received: %s", std::string(msg).c_str());
}

void task(void *pvParams) {
  StreamBufferHandle_t sbuf = static_cast<StreamBufferHandle_t>(pvParams);
  MidiParser parser(cbk);
  uint8_t buffer[64];
  size_t read = 0;

  while (true) {
    read = xStreamBufferReceive(sbuf, buffer, sizeof(buffer), xTicksToWait);
    parser.feed(buffer, read);
  }
}

void receive(void *pvParams) {
  SynthChannel ch({}, notes);
  MidiChannelMessage msg;
  int64_t time = esp_timer_get_time();
  while (true) {
    BaseType_t status = xQueueReceive(xQueue, &msg, xTicksToWait);
    int64_t now = esp_timer_get_time();
    Duration delta = Duration::micros(now - time);

    if (status) {
      ch.handle(msg, delta);
      time = now;
    }
  }
}

void play(StreamBufferHandle_t sbuf) {
  xTaskCreatePinnedToCore(task, "Synth", 8 * 1024, sbuf, 1, NULL, 1);
}
