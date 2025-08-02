#ifndef SENSORDATA_H
#define SENSORDATA_H

#include <Arduino.h>  // For QueueHandle_t

// Enum to identify the type of data in the queue
enum DataType { SENSORS_DATA, ADC_DATA };

// Struct for BME680 sensor data
struct BME680_Data {
  float temperature;
  float humidity;
  float pressure;
  float gas_resistance;
};

// Struct for SHT31 sensor data
struct SHT31_Data {
  float temperature;
  float humidity;
};

// Struct for ADC data
struct ADC_Data {
  uint16_t value;
  int channel;
  long frequency;
};

// Struct to be sent over the queue
struct DataPacket {
  DataType type;
  union {
    struct {
      BME680_Data bmeData;
      SHT31_Data sht31Data;
      uint16_t mq3_value;
      uint16_t mq135_value;
      uint16_t mq136_value;
      uint16_t mq137_value;
    } sensorData;
    ADC_Data adcData;
  };
};

#endif  // SENSORDATA_H
