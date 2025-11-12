#include "synth_config.hpp"
#include "core.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "notes.hpp"
#include "nvs.h"
#include "string"
#include <cstdint>
#include <cstring>

static const char *TAG = "synth_config";

nvs_handle_t handle;
static Config config_;

Config &load_config() {
  // Open NVS handle
  ESP_LOGI(TAG, "\nOpening Non-Volatile Storage (NVS) handle...");
  esp_err_t err = nvs_open("synth", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  } else {
    uint8_t u8;
    uint32_t u32;

    if (nvs_get_u8(handle, "notes", &u8) == ESP_OK)
      config_.notes = u8;

    if (nvs_get_u32(handle, "a440", &u32) == ESP_OK)
      config_.a440 = Hertz(u32);

    if (nvs_get_u32(handle, "min_on", &u32) == ESP_OK)
      config_.min_on_time = Duration32::micros(u32);

    if (nvs_get_u32(handle, "min_dead", &u32) == ESP_OK)
      config_.min_deadtime = Duration32::micros(u32);

    if (nvs_get_u32(handle, "max_on", &u32) == ESP_OK)
      config_.max_on_time = Duration32::micros(u32);
  }
  ESP_LOGI(TAG, "Config:\n%s\n", std::string(config_).c_str());
  return config_;
}

Config &get_config() { return config_; }

void reset_config() {
  config_ = {};
  save_config();
}
void save_config() {
  nvs_set_u8(handle, "notes", config_.notes);

  nvs_set_u32(handle, "a440", config_.a440);
  nvs_set_u32(handle, "min_on", config_.min_on_time.micros());
  nvs_set_u32(handle, "min_dead", config_.min_deadtime.micros());
  nvs_set_u32(handle, "max_on", config_.max_on_time.micros());

  esp_err_t err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) while saving configuration!",
             esp_err_to_name(err));
  }
}
