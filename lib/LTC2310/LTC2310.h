#ifndef LTC2310_H
#define LTC2310_H

#include <Arduino.h>
#include <SPI.h>

class LTC2310 {
 private:
  int csPin;
  SPIClass &spi;

 public:
  LTC2310(int csPin, SPIClass &spi = SPI);
  void init();
  uint16_t readValue();
};

#endif  // LTC2310_H
