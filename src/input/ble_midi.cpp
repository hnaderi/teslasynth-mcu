#include "ble_midi.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/idf_additions.h"
#include <NimBLEDevice.h>

// As specified in
// Specification for MIDI over Bluetooth Low Energy (BLE-MIDI)
// Version 1.0a, November 1, 2015
// 3. BLE Service and Characteristics Definitions
static const char *const SERVICE_UUID = "03b80e5a-ede8-4b33-a751-6ce34ec4c700";
static const char *const CHARACTERISTIC_UUID =
    "7772e5db-3868-4112-a1a9-f2669d106bf3";

static const char *TAG = "BLE_MIDI";

ESP_EVENT_DEFINE_BASE(EVENT_BLE_BASE);

namespace teslasynth::app::devices::ble_midi {
class MIDIServerCallbacks : public BLEServerCallbacks {
public:
  MIDIServerCallbacks() {}

protected:
  void onConnect(BLEServer *, NimBLEConnInfo &) {
    ESP_LOGI(TAG, "Connected!");
    ESP_ERROR_CHECK(esp_event_post(EVENT_BLE_BASE, BLE_DEVICE_CONNECTED, NULL,
                                   0, portMAX_DELAY));
  };

  void onDisconnect(BLEServer *, NimBLEConnInfo &, int) {
    ESP_LOGI(TAG, "Disconnected!");
    ESP_ERROR_CHECK(esp_event_post(EVENT_BLE_BASE, BLE_DEVICE_DISCONNECTED,
                                   NULL, 0, portMAX_DELAY));
  }
};

class MIDICharacteristicCallbacks : public BLECharacteristicCallbacks {
  StreamBufferHandle_t sbuf;

public:
  MIDICharacteristicCallbacks(StreamBufferHandle_t buffer) : sbuf(buffer) {}

protected:
  void onWrite(BLECharacteristic *characteristic, NimBLEConnInfo &) {
    auto rxValue = characteristic->getValue();
    if (rxValue.length() > 0) {
      if (xStreamBufferSend(sbuf, rxValue.begin(), rxValue.length(), 0) !=
          rxValue.length()) {
        ESP_LOGE(TAG, "Couldn't write received BLE data!");
      }
    }
  }
};

StreamBufferHandle_t init() {
  NimBLEDevice::init(CONFIG_TESLASYNTH_DEVICE_NAME);

  auto sbuf = xStreamBufferCreate(256, 1);
  if (sbuf == nullptr) {
    ESP_LOGE(TAG, "Couldn't allocate BLE stream buffer!");
    return nullptr;
  }

  NimBLEDevice::setSecurityAuth(false, true, true);

  auto _server = NimBLEDevice::createServer();
  _server->setCallbacks(new MIDIServerCallbacks());
  _server->advertiseOnDisconnect(true);

  auto service = _server->createService(BLEUUID(SERVICE_UUID));

  auto _characteristic = service->createCharacteristic(
      BLEUUID(CHARACTERISTIC_UUID),
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY |
          NIMBLE_PROPERTY::WRITE_NR);

  _characteristic->setCallbacks(new MIDICharacteristicCallbacks(sbuf));

  service->start();

  auto _advertising = _server->getAdvertising();
  _advertising->addServiceUUID(service->getUUID());
  _advertising->setName(CONFIG_TESLASYNTH_DEVICE_NAME);
  _advertising->start();

  return sbuf;
}

} // namespace teslasynth::app::devices::ble_midi
