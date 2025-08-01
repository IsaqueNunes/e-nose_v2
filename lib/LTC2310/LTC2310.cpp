#include "LTC2310.h"

LTC2310::LTC2310(int csPin, SPIClass &spi) : csPin(csPin), spi(spi) { }

void LTC2310::init() {
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);
  spi.begin();
}

uint16_t LTC2310::readValue() {
  spi.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  digitalWrite(csPin, LOW);
  uint16_t value = spi.transfer16(0x0000);
  digitalWrite(csPin, HIGH);
  spi.endTransaction();
  return value;
}
