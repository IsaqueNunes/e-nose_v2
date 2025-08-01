#ifndef BME680_SENSOR_H
#define BME680_SENSOR_H

#include <Adafruit_BME680.h>

#include "SensorData.h"

class BME680_Sensor {
 private:
  Adafruit_BME680 bme;

 public:
  BME680_Sensor();
  bool init();
  bool readSensor(BME680_Data &data);
};

#endif  // BME680_SENSOR_H
