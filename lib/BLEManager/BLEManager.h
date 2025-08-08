#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "SensorData.h"

#define SERVICE_UUID "bea5692f-939d-4e5a-bfa9-80d3efb8e3cb"
#define CHARACTERISTIC_UUID "b13493c7-5499-4b0a-a3d9-66eea53f382c"

/**
 * @class BLEManager
 * @brief Manages all Bluetooth Low Energy (BLE) functionality.
 *
 * This class encapsulates the setup of the BLE server, service, and
 * characteristic, as well as handling connections and sending data.
 */
class BLEManager {
 public:
  /**
   * @brief Construct a new BLEManager object.
   *
   * @param deviceName The name the ESP32 will advertise over BLE.
   */
  BLEManager(const std::string& deviceName);

  /**
   * @brief Initializes the BLE stack, server, service, and characteristic.
   * Starts advertising the device.
   */
  void init();

  /**
   * @brief Sends a DataPacket over BLE if a client is connected.
   *
   * @param packet The DataPacket to be sent.
   */
  void sendData(const DataPacket& packet);

 private:
  BLECharacteristic* pCharacteristic;
  bool deviceConnected;
  std::string deviceName;

  /**
   * @class ServerCallbacks
   * @brief Handles BLE client connection and disconnection events.
   */
  class ServerCallbacks : public BLEServerCallbacks {
   public:
    /**
     * @brief Pointer to the parent BLEManager's connection status flag.
     */
    bool* connectedFlag;

    /**
     * @brief Construct a new ServerCallbacks object.
     * @param flag Pointer to the boolean flag indicating connection status.
     */
    ServerCallbacks(bool* flag) : connectedFlag(flag) { }

    /**
     * @brief Called when a BLE client connects.
     * @param pServer A pointer to the BLE server instance.
     */
    void onConnect(BLEServer* pServer) override;

    /**
     * @brief Called when a BLE client disconnects.
     * @param pServer A pointer to the BLE server instance.
     */
    void onDisconnect(BLEServer* pServer) override;
  };
};

#endif  // BLE_MANAGER_H
