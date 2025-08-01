#ifndef SHT31_SENSOR_H
#define SHT31_SENSOR_H

#include "Adafruit_SHT31.h"
#include "SensorData.h"

class SHT31_Sensor {
 private:
  Adafruit_SHT31 sht31;

 public:
  SHT31_Sensor();
  bool init();
  bool readSensor(SHT31_Data &data);
};

#endif  // SHT31_SENSOR_H
