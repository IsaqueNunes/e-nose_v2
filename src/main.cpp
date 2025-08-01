#include <Arduino.h>
#include <Multiplexer.h>
#include <WaveGenerator.h>

#include "BME680_Sensor.h"
#include "LTC2310.h"
#include "SHT31_Sensor.h"
#include "SensorData.h"

// Definições dos Pinos
#define LTC2310_CS_PIN 4
#define MQ3_PIN 34
#define MQ135_PIN 35
#define MQ136_PIN 33
#define MQ137_PIN 32

// Instâncias dos objetos
WaveGenerator waveGenerator({1000, 2000, 3000});
Multiplexer multiplexer({27, 25, 26, 13}, {1000, 2000, 3000}, 1000);
LTC2310 adc(LTC2310_CS_PIN);
BME680_Sensor bmeSensor;
SHT31_Sensor sht31Sensor;

// Tarefas do FreeRTOS
TaskHandle_t WaveMultiplexerTask;
TaskHandle_t SensorReaderTask;

// Função da Tarefa para o Core 1 (Geração de Onda e Multiplexer)
void waveMultiplexerCode(void *pvParameters) {
  Serial.print("Task for WaveGenerator and Multiplexer running on core ");
  Serial.println(xPortGetCoreID());
  multiplexer.init();
  for (;;) {
    multiplexer.update();
    vTaskDelay(10);
  }
}

// Função da Tarefa para o Core 0 (Leitura dos Sensores)
void sensorReaderCode(void *pvParameters) {
  Serial.print("Task for Sensor reading running on core ");
  Serial.println(xPortGetCoreID());

  adc.init();
  bmeSensor.init();
  sht31Sensor.init();

  pinMode(MQ3_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);
  pinMode(MQ136_PIN, INPUT);
  pinMode(MQ137_PIN, INPUT);

  for (;;) {
    BME680_Data bmeData;
    SHT31_Data sht31Data;

    // Leitura dos sensores
    if (bmeSensor.readSensor(bmeData)) {
      Serial.printf(
          "BME680 -> Temp: %.2f *C | Hum: %.2f %% | Pres: %.2f hPa | Gas: %.2f "
          "KOhms\n",
          bmeData.temperature,
          bmeData.humidity,
          bmeData.pressure,
          bmeData.gas_resistance
      );
    }

    if (sht31Sensor.readSensor(sht31Data)) {
      Serial.printf(
          "SHT31 -> Temp: %.2f *C | Hum: %.2f %%\n",
          sht31Data.temperature,
          sht31Data.humidity
      );
    }

    // Leitura dos sensores de gás (ADC)
    uint16_t mq3_value = analogRead(MQ3_PIN);
    uint16_t mq135_value = analogRead(MQ135_PIN);
    uint16_t mq136_value = analogRead(MQ136_PIN);
    uint16_t mq137_value = analogRead(MQ137_PIN);

    // Leitura do ADC LTC2310
    uint16_t adc_value = adc.readValue();

    Serial.printf(
        "MQ3: %d | MQ135: %d | MQ136: %d | MQ137: %d | LTC2310: %d \n",
        mq3_value,
        mq135_value,
        mq136_value,
        mq137_value,
        adc_value
    );

    Serial.println("---------------------------------------------------");
    vTaskDelay(2000);
  }
}

void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
      waveMultiplexerCode,   /* Function to implement the task */
      "WaveMultiplexerTask", /* Name of the task */
      10000,                 /* Stack size in words */
      NULL,                  /* Task input parameter */
      1,                     /* Priority of the task */
      &WaveMultiplexerTask,  /* Task handle. */
      1
  ); /* Core where the task should run */

  xTaskCreatePinnedToCore(
      sensorReaderCode,   /* Function to implement the task */
      "SensorReaderTask", /* Name of the task */
      10000,              /* Stack size in words */
      NULL,               /* Task input parameter */
      1,                  /* Priority of the task */
      &SensorReaderTask,  /* Task handle. */
      0
  ); /* Core where the task should run */
}

void loop() {
  // O loop principal fica vazio, pois as tarefas cuidam de todo o trabalho
  vTaskDelete(NULL);
}
