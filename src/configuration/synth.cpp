#include "synth.hpp"
#include "core.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "notes.hpp"
#include "nvs.h"
#include "synthesizer_events.hpp"
#include <cstdint>
#include <cstring>
#include <optional>

namespace teslasynth::app::configuration {
static const char *TAG = "synth_config";
using namespace synth;

namespace keys {
static constexpr const char *max_on_time = "max-on";
static constexpr const char *min_deadtime = "min-dead";
// static constexpr const char *tuning = "tuning";
static constexpr const char *notes = "notes";
static constexpr const char *instrument = "instrument";
}; // namespace keys

nvs_handle_t handle;
static Config config_;

static bool init_nvs_handle() {
  if (handle)
    return true;
  ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle...");
  esp_err_t err = nvs_open("synth", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return false;
  }
  return true;
}

const Config &load_config() {
  if (init_nvs_handle()) {
    int8_t i8;
    uint16_t u16;

    if (nvs_get_i8(handle, keys::notes, &i8) == ESP_OK)
      config_.notes = i8;

    if (nvs_get_i8(handle, keys::instrument, &i8) == ESP_OK)
      config_.instrument =
          i8 > 0 ? std::optional<uint8_t>(i8) : std::optional<uint8_t>();

    // if (nvs_get_u32(handle, keys::tuning, &u32) == ESP_OK)
    //   config_.a440 = Hertz(u32);

    if (nvs_get_u16(handle, keys::min_deadtime, &u16) == ESP_OK)
      config_.min_deadtime = Duration16::micros(u16);

    if (nvs_get_u16(handle, keys::max_on_time, &u16) == ESP_OK)
      config_.max_on_time = Duration16::micros(u16);
  }
  return config_;
}

const Config &get_config() { return config_; }

void update_config(const Config &config) {
  config_ = config;
  ESP_ERROR_CHECK(esp_event_post(EVENT_SYNTHESIZER_BASE,
                                 SYNTHESIZER_CONFIG_UPDATED, NULL, 0,
                                 portMAX_DELAY));
}

void reset_config() {
  update_config({});
  save_config();
}
void save_config() {
  nvs_set_i8(handle, keys::notes, config_.notes);
  if (config_.instrument.has_value()) {
    nvs_set_i8(handle, keys::instrument, *config_.instrument);
  } else {
    nvs_set_i8(handle, keys::instrument, -1);
  }

  // nvs_set_u32(handle, keys::tuning, config_.a440);
  nvs_set_u16(handle, keys::max_on_time, config_.max_on_time.micros());
  nvs_set_u16(handle, keys::min_deadtime, config_.min_deadtime.micros());

  esp_err_t err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) while saving configuration!",
             esp_err_to_name(err));
  }
}

} // namespace teslasynth::app::configuration
