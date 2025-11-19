#include "teslasynth.hpp"
#include "configuration/synth.hpp"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"

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
  load_config();
  ESP_ERROR_CHECK(esp_event_loop_create_default());

#if CONFIG_TESLASYNTH_GUI_ENABLED
  init_gui();
#endif
  init_cli();
  auto sbuf = init_ble_midi();
  init_synth(sbuf);
  while (1) {
    vTaskDelay(portMAX_DELAY);
  }
}
