#ifndef SENSORDATA_H
#define SENSORDATA_H

// Estrutura para os dados do sensor BME680
struct BME680_Data {
  float temperature;
  float humidity;
  float pressure;
  float gas_resistance;
};

// Estrutura para os dados do sensor SHT31
struct SHT31_Data {
  float temperature;
  float humidity;
};

#endif  // SENSORDATA_H
