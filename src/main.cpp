#include <Arduino.h>

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

// System Parameters
const int WAVE_SETTLING_TIME_US = 100;
const int SENSOR_READ_INTERVAL_MS = 2000;
const std::initializer_list<long> FREQUENCIES_HZ = {
    10, 100, 1000, 10000, 25000, 50000
};
const int NUM_MUX_CHANNELS = 4;

// Data Queue
#define DATA_QUEUE_LENGTH 50
QueueHandle_t dataQueue;

// Object Instances
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

// FreeRTOS Task Handles
TaskHandle_t sensorReaderTaskHandle;
TaskHandle_t loggerTaskHandle;

// Task for reading all sensors and performing measurements (Core 0)
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

    // 1. Read slow sensors once per cycle into temporary structs
    BME680_Data bmeData;
    SHT31_Data sht31Data;
    bmeSensor.readSensor(bmeData);
    sht31Sensor.readSensor(sht31Data);

    // Copy data from temporary structs to the main packet
    packet.bme_temperature = bmeData.temperature;
    packet.bme_humidity = bmeData.humidity;
    packet.bme_pressure = bmeData.pressure;
    packet.bme_gas_resistance = bmeData.gas_resistance;
    packet.sht_temperature = sht31Data.temperature;
    packet.sht_humidity = sht31Data.humidity;

    // Read analog sensors directly into the packet
    packet.mq3_value = analogRead(MQ3_PIN);
    packet.mq135_value = analogRead(MQ135_PIN);
    packet.mq136_value = analogRead(MQ136_PIN);
    packet.mq137_value = analogRead(MQ137_PIN);

    // 2. Loop through all frequencies and channels
    for (int freq_idx = 0; freq_idx < FREQUENCIES_HZ.size(); ++freq_idx) {
      for (int ch = 1; ch <= NUM_MUX_CHANNELS; ++ch) {
        // Perform the RMS measurement
        packet.adc_rms = controller.measureRms(freq_idx, ch);
        packet.adc_channel = ch;
        packet.adc_frequency = waveGenerator.getFrequencyByIndex(freq_idx);

        // Send a complete data packet for this specific measurement
        xQueueSend(dataQueue, &packet, portMAX_DELAY);
      }
    }

    // 3. Wait for the next measurement cycle
    vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
  }
}

// Task for logging data to Serial (Core 1)
void loggerTask(void *pvParameters) {
  Serial.print("Logger Task running on core ");
  Serial.println(xPortGetCoreID());

  DataPacket receivedPacket;

  // Print CSV header
  Serial.println(
      "ADC_RMS,ADC_Channel,ADC_Frequency,BME_Temp,BME_Hum,BME_Pres,BME_Gas,SHT_"
      "Temp,SHT_Hum,MQ3,MQ135,MQ136,MQ137"
  );

  for (;;) {
    if (xQueueReceive(dataQueue, &receivedPacket, portMAX_DELAY) == pdPASS) {
      Serial.printf(
          "%.2f,%d,%ld,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%d,%d\n",
          receivedPacket.adc_rms,
          receivedPacket.adc_channel,
          receivedPacket.adc_frequency,
          receivedPacket.bme_temperature,
          receivedPacket.bme_humidity,
          receivedPacket.bme_pressure,
          receivedPacket.bme_gas_resistance,
          receivedPacket.sht_temperature,
          receivedPacket.sht_humidity,
          receivedPacket.mq3_value,
          receivedPacket.mq135_value,
          receivedPacket.mq136_value,
          receivedPacket.mq137_value
      );
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  dataQueue = xQueueCreate(DATA_QUEUE_LENGTH, sizeof(DataPacket));
  if (dataQueue == NULL) {
    Serial.println("Error creating the data queue");
    while (1);
  }

  // Create Sensor Reader Task on Core 0 (Handles I/O and processing)
  xTaskCreatePinnedToCore(
      sensorReaderTask,
      "SensorReaderTask",
      10000,
      NULL,
      1,
      &sensorReaderTaskHandle,
      0
  );

  // Create Logger Task on Core 1 (Handles Serial output)
  xTaskCreatePinnedToCore(
      loggerTask, "LoggerTask", 4096, NULL, 1, &loggerTaskHandle, 1
  );
}

void loop() { vTaskDelete(NULL); }
