#include "app.hpp"
#include "cli.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include <input/ble_midi.hpp>

void initialize_nvs() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
}

extern "C" void app_main(void) {
  initialize_nvs();

  init_cli();
  auto sbuf = ble_begin();
  play(sbuf);
  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
