#include "BLEManager.h"

void BLEManager::ServerCallbacks::onConnect(BLEServer* pServer) {
  *connectedFlag = true;
  Serial.println("BLE Client Connected");
}

void BLEManager::ServerCallbacks::onDisconnect(BLEServer* pServer) {
  *connectedFlag = false;
  Serial.println("BLE Client Disconnected");
  pServer->getAdvertising()->start();
}

BLEManager::BLEManager(const std::string& deviceName)
    : pCharacteristic(nullptr),
      deviceConnected(false),
      deviceName(deviceName) { }

void BLEManager::init() {
  BLEDevice::init(deviceName);

  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks(&deviceConnected));

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("Waiting for a client connection to notify...");
}

void BLEManager::sendData(const DataPacket& packet) {
  if (deviceConnected && pCharacteristic != nullptr) {
    pCharacteristic->setValue((uint8_t*) &packet, sizeof(DataPacket));
    pCharacteristic->notify();
  }
}
