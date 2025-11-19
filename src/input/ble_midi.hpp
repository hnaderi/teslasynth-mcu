#pragma once

#include "esp_event_base.h"

ESP_EVENT_DECLARE_BASE(EVENT_BLE_BASE);
enum {
  BLE_DEVICE_CONNECTED,
  BLE_DEVICE_DISCONNECTED,
};
