#include "SHT31_Sensor.h"

SHT31_Sensor::SHT31_Sensor() : sht31(Adafruit_SHT31()) { }

bool SHT31_Sensor::init() {
  if (!sht31.begin(0x44)) {
    Serial.println("Couldn't find SHT31");
    return false;
  }
  return true;
}

bool SHT31_Sensor::readSensor(SHT31_Data &data) {
  data.temperature = sht31.readTemperature();
  data.humidity = sht31.readHumidity();

  if (!isnan(data.temperature) && !isnan(data.humidity)) {
    return true;
  }
  return false;
}
