#ifndef SENSORDATA_H
#define SENSORDATA_H

/**
 * @file SensorData.h
 * @brief Defines the data structures for sensor readings and data packets.
 */

// --- Configuração do Sensor Fabricado ---
// Altere estes valores para corresponder ao seu setup em main.cpp
#define NUM_FREQUENCIAS 6
#define NUM_CANAIS 4
#define ADC_DATA_POINTS (NUM_FREQUENCIAS * NUM_CANAIS)

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
 * @struct LockInResult
 * @brief Holds the result of a lock-in measurement for one point.
 */
struct LockInResult {
  float mean;     // Média das amplitudes calculadas
  float std_dev;  // Desvio padrão das amplitudes
};

/**
 * @struct DataPacket
 * @brief A comprehensive packet containing all sensor data for a single
 * measurement cycle. This is the structure that is sent through the FreeRTOS
 * queue and over BLE.
 */
#pragma pack(push, 1)  // Garante o empacotamento sem padding
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
  float mq3_value;
  float mq135_value;
  float mq136_value;
  float mq137_value;

  // Processed ADC Data from custom sensor
  // Arrays para armazenar a média e o desvio padrão de cada combinação
  // Frequência x Canal
  float adc_mean[ADC_DATA_POINTS];
  float adc_std_dev[ADC_DATA_POINTS];
};
#pragma pack(pop)

#endif  // SENSORDATA_H
