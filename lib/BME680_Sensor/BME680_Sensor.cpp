#include "BME680_Sensor.h"

BME680_Sensor::BME680_Sensor() { }

bool BME680_Sensor::init() {
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    return false;
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // 320*C for 150 ms

  return true;
}

bool BME680_Sensor::readSensor(BME680_Data &data) {
  if (!bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return false;
  }
  data.temperature = bme.temperature;
  data.pressure = bme.pressure / 100.0;
  data.humidity = bme.humidity;
  data.gas_resistance = bme.gas_resistance / 1000.0;
  return true;
}
