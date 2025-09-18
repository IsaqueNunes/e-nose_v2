#ifndef SENSORDATA_H
#define SENSORDATA_H

/**
 * @file SensorData.h
 * @brief Defines the data structures for sensor readings and data packets.
 */

/**
 * @struct BME680_Data
 * @brief Holds the data read from the BME680 sensor.
 */
struct BME680_Data {
  float temperature;
  float humidity;
  float pressure;
  float gas_resistance;
};

/**
 * @struct SHT31_Data
 * @brief Holds the data read from the SHT31 sensor.
 */
struct SHT31_Data {
  float temperature;
  float humidity;
};

/**
 * @struct DataPacket
 * @brief A comprehensive packet containing all sensor data for a single
 * measurement point. This is the structure that is sent through the FreeRTOS
 * queue.
 */
struct DataPacket {
  // BME680 Data
  float bme_temperature;
  float bme_humidity;
  float bme_pressure;
  float bme_gas_resistance;

  // SHT31 Data
  float sht_temperature;
  float sht_humidity;

  // MQ Sensor Data (Analog)
  // uint16_t mq3_value;
  // uint16_t mq135_value;
  // uint16_t mq136_value;
  // uint16_t mq137_value;
  float mq3_value;
  float mq135_value;
  float mq136_value;
  float mq137_value;

  // Processed ADC Data
  float adc_rms;
  int adc_channel;
  long adc_frequency;
};

#endif  // SENSORDATA_H
