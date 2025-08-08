#include <Arduino.h>

#include "BLEManager.h"
#include "BME680_Sensor.h"
#include "ENoseController.h"
#include "LTC2310.h"
#include "Multiplexer.h"
#include "SHT31_Sensor.h"
#include "SensorData.h"
#include "WaveGenerator.h"

// Pin Definitions
#define LTC2310_CS_PIN 4
#define MQ3_PIN 34
#define MQ135_PIN 35
#define MQ136_PIN 33
#define MQ137_PIN 32
#define WAVEGEN_DATA_PIN 23
#define WAVEGEN_CLOCK_PIN 18
#define WAVEGEN_FSYNC_PIN 5

const int WAVE_SETTLING_TIME_US = 100;
const int SENSOR_READ_INTERVAL_MS = 2000;
const std::initializer_list<long> FREQUENCIES_HZ = {
    10, 100, 1000, 10000, 100000, 1000000
};
const int NUM_MUX_CHANNELS = 4;

#define DATA_QUEUE_LENGTH 50
QueueHandle_t dataQueue;

WaveGenerator waveGenerator(
    FREQUENCIES_HZ, WAVEGEN_DATA_PIN, WAVEGEN_CLOCK_PIN, WAVEGEN_FSYNC_PIN
);
Multiplexer multiplexer({27, 25, 26, 13});
LTC2310 adc(LTC2310_CS_PIN);
BME680_Sensor bmeSensor;
SHT31_Sensor sht31Sensor;
ENoseController controller(
    waveGenerator, multiplexer, adc, WAVE_SETTLING_TIME_US
);
BLEManager bleManager("E-Nose");

TaskHandle_t sensorReaderTaskHandle;
TaskHandle_t dataTransferTaskHandle;

void sensorReaderTask(void *pvParameters) {
  Serial.print("Sensor Reader Task running on core ");
  Serial.println(xPortGetCoreID());

  controller.init();
  bmeSensor.init();
  sht31Sensor.init();

  pinMode(MQ3_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);
  pinMode(MQ136_PIN, INPUT);
  pinMode(MQ137_PIN, INPUT);

  for (;;) {
    DataPacket packet;

    BME680_Data bmeData;
    SHT31_Data sht31Data;
    bmeSensor.readSensor(bmeData);
    sht31Sensor.readSensor(sht31Data);

    packet.bme_temperature = bmeData.temperature;
    packet.bme_humidity = bmeData.humidity;
    packet.bme_pressure = bmeData.pressure;
    packet.bme_gas_resistance = bmeData.gas_resistance;
    packet.sht_temperature = sht31Data.temperature;
    packet.sht_humidity = sht31Data.humidity;
    packet.mq3_value = analogRead(MQ3_PIN);
    packet.mq135_value = analogRead(MQ135_PIN);
    packet.mq136_value = analogRead(MQ136_PIN);
    packet.mq137_value = analogRead(MQ137_PIN);

    for (int freq_idx = 0; freq_idx < FREQUENCIES_HZ.size(); ++freq_idx) {
      for (int ch = 1; ch <= NUM_MUX_CHANNELS; ++ch) {
        packet.adc_rms = controller.measureRms(freq_idx, ch);
        packet.adc_channel = ch;
        packet.adc_frequency = waveGenerator.getFrequencyByIndex(freq_idx);
        xQueueSend(dataQueue, &packet, portMAX_DELAY);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
  }
}

void dataTransferTask(void *pvParameters) {
  Serial.print("Data Transfer Task running on core ");
  Serial.println(xPortGetCoreID());

  DataPacket receivedPacket;

  for (;;) {
    if (xQueueReceive(dataQueue, &receivedPacket, portMAX_DELAY) == pdPASS) {
      bleManager.sendData(receivedPacket);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting E-Nose...");

  bleManager.init();

  dataQueue = xQueueCreate(DATA_QUEUE_LENGTH, sizeof(DataPacket));
  if (dataQueue == NULL) {
    Serial.println("Error creating the data queue");
    while (1);
  }

  xTaskCreatePinnedToCore(
      sensorReaderTask,
      "SensorReaderTask",
      10000,
      NULL,
      1,
      &sensorReaderTaskHandle,
      0
  );

  xTaskCreatePinnedToCore(
      dataTransferTask,
      "DataTransferTask",
      4096,
      NULL,
      1,
      &dataTransferTaskHandle,
      1
  );
}

void loop() { vTaskDelete(NULL); }
