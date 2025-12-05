#include "synth.hpp"
#include "esp_err.h"
#include "esp_log.h"
#include "nvs.h"
#include <cstring>

namespace teslasynth::app::configuration {
static const char *TAG = "synth_config";
static const char *KEY = "config";
using namespace core;

static esp_err_t init(nvs_handle_t &handle) {
  esp_err_t err = nvs_open("synth", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
  }
  return err;
}

AppConfig read() {
  nvs_handle_t handle;
  ESP_ERROR_CHECK(init(handle));
  AppConfig config;

  size_t read_size = sizeof(config);
  auto err = nvs_get_blob(handle, KEY, &config, &read_size);
  if (err != ESP_OK || read_size != sizeof(config)) {
    ESP_LOGE(TAG, "Corrupted configuration!");
    config = AppConfig();
  }

  nvs_close(handle);
  return config;
}

void persist(UIHandle &ui) {
  static_assert(std::is_trivially_copyable<AppConfig>::value,
                "AppConfig must be trivially copyable");

  nvs_handle_t handle;
  ESP_ERROR_CHECK(init(handle));

  AppConfig config = ui.config_read();
  auto err = nvs_set_blob(handle, KEY, &config, sizeof(config));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Couldn't persist configuration!");
  } else {
    nvs_commit(handle);
  }

  nvs_close(handle);
}
} // namespace teslasynth::app::configuration
