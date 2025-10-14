#include "ble_midi.hpp"
#include "esp_log.h"
#include <NimBLEDevice.h>

// As specified in
// Specification for MIDI over Bluetooth Low Energy (BLE-MIDI)
// Version 1.0a, November 1, 2015
// 3. BLE Service and Characteristics Definitions
static const char *const SERVICE_UUID = "03b80e5a-ede8-4b33-a751-6ce34ec4c700";
static const char *const CHARACTERISTIC_UUID =
    "7772e5db-3868-4112-a1a9-f2669d106bf3";

static const char *TAG = "BLE_MIDI";

class MIDIServerCallbacks : public BLEServerCallbacks {
public:
  MIDIServerCallbacks() {}

protected:
  void onConnect(BLEServer *server, NimBLEConnInfo &conn) {
    ESP_LOGI(TAG, "Connected!");
  };

  void onDisconnect(BLEServer *server, NimBLEConnInfo &conn, int) {
    ESP_LOGI(TAG, "Disconnected!");
  }
};

class MIDICharacteristicCallbacks : public BLECharacteristicCallbacks {
public:
  MIDICharacteristicCallbacks() {}

protected:
  void onWrite(BLECharacteristic *characteristic, NimBLEConnInfo &connection) {
    std::string rxValue = characteristic->getValue();
    if (rxValue.length() > 0) {
      ESP_LOGI(TAG, "Received:");
      ESP_LOG_BUFFER_HEX_LEVEL(TAG, rxValue.data(), rxValue.length(),
                               ESP_LOG_INFO);
    }
  }
};

bool ble_begin(const char *deviceName) {
  BLEDevice::init(deviceName);

  /**
   * Set the IO capabilities of the device, each option will trigger a different
   * pairing method. BLE_HS_IO_DISPLAY_ONLY    - Passkey pairing
   *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
   *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
   */
  // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // use passkey
  // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric
  // comparison

  /**
   *  2 different ways to set security - both calls achieve the same result.
   *  no bonding, no man in the middle protection, BLE secure connections.
   *
   *  These are the default values, only shown here for demonstration.
   */
  // NimBLEDevice::setSecurityAuth(false, false, true);

  //    NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND |
  //    BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

  NimBLEDevice::setSecurityAuth(true, false, false);

  auto _server = BLEDevice::createServer();
  _server->setCallbacks(new MIDIServerCallbacks());
  _server->advertiseOnDisconnect(true);

  // Create the BLE Service
  auto service = _server->createService(BLEUUID(SERVICE_UUID));

  // Create a BLE Characteristic
  auto _characteristic = service->createCharacteristic(
      BLEUUID(CHARACTERISTIC_UUID),
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY |
          NIMBLE_PROPERTY::WRITE_NR);

  _characteristic->setCallbacks(new MIDICharacteristicCallbacks());

  // Start the service
  service->start();

  // Start advertising
  auto _advertising = _server->getAdvertising();
  _advertising->addServiceUUID(service->getUUID());
  _advertising->setAppearance(0x00);
  _advertising->setName(deviceName);
  _advertising->start();

  return true;
}
