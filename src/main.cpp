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
const int CHANNEL_DURATION_MS =
    500;  // Time each channel is active for a frequency
const int WAVE_SETTLING_TIME_US =
    100;  // Time for wave to settle after frequency change
const int SENSOR_READ_INTERVAL_MS = 2000;  // Interval for reading slow sensors

// Data Queue
#define DATA_QUEUE_LENGTH 50
QueueHandle_t dataQueue;

// Object Instances
WaveGenerator waveGenerator(
    {10, 100, 1000, 10000, 100000, 1000000},
    WAVEGEN_DATA_PIN,
    WAVEGEN_CLOCK_PIN,
    WAVEGEN_FSYNC_PIN
);
Multiplexer multiplexer({27, 25, 26, 13});
LTC2310 adc(LTC2310_CS_PIN);
BME680_Sensor bmeSensor;
SHT31_Sensor sht31Sensor;

ENoseController controller(
    waveGenerator,
    multiplexer,
    adc,
    dataQueue,
    CHANNEL_DURATION_MS,
    WAVE_SETTLING_TIME_US
);

// FreeRTOS Task Handles
TaskHandle_t controllerTaskHandle;
TaskHandle_t sensorReaderTaskHandle;
TaskHandle_t loggerTaskHandle;

// Task for the main controller (Core 1)
void controllerTask(void *pvParameters) {
  Serial.print("Controller Task running on core ");
  Serial.println(xPortGetCoreID());
  controller.init();
  for (;;) {
    controller.update();
    vTaskDelay(1);  // Small delay to prevent watchdog timeout
  }
}

// Task for reading slow sensors (Core 0)
void sensorReaderTask(void *pvParameters) {
  Serial.print("Sensor Reader Task running on core ");
  Serial.println(xPortGetCoreID());

  bmeSensor.init();
  sht31Sensor.init();

  pinMode(MQ3_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);
  pinMode(MQ136_PIN, INPUT);
  pinMode(MQ137_PIN, INPUT);

  for (;;) {
    DataPacket packet;
    packet.type = SENSORS_DATA;

    bmeSensor.readSensor(packet.sensorData.bmeData);
    sht31Sensor.readSensor(packet.sensorData.sht31Data);

    packet.sensorData.mq3_value = analogRead(MQ3_PIN);
    packet.sensorData.mq135_value = analogRead(MQ135_PIN);
    packet.sensorData.mq136_value = analogRead(MQ136_PIN);
    packet.sensorData.mq137_value = analogRead(MQ137_PIN);

    xQueueSend(dataQueue, &packet, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
  }
}

// Task for logging data to Serial (Core 0)
void loggerTask(void *pvParameters) {
  Serial.print("Logger Task running on core ");
  Serial.println(xPortGetCoreID());

  DataPacket receivedPacket;

  // Print CSV header
  Serial.println(
      "DataType,Value,Channel,Frequency,Temp_BME,Hum_BME,Pres_BME,Gas_BME,Temp_"
      "SHT,Hum_SHT,MQ3,MQ135,MQ136,MQ137"
  );

  for (;;) {
    if (xQueueReceive(dataQueue, &receivedPacket, portMAX_DELAY) == pdPASS) {
      if (receivedPacket.type == ADC_DATA) {
        Serial.printf(
            "ADC,%d,%d,%ld,,,,,,,,,,,\n",
            receivedPacket.adcData.value,
            receivedPacket.adcData.channel,
            receivedPacket.adcData.frequency
        );
      } else if (receivedPacket.type == SENSORS_DATA) {
        Serial.printf(
            "SENSORS,,,,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%d,%d\n",
            receivedPacket.sensorData.bmeData.temperature,
            receivedPacket.sensorData.bmeData.humidity,
            receivedPacket.sensorData.bmeData.pressure,
            receivedPacket.sensorData.bmeData.gas_resistance,
            receivedPacket.sensorData.sht31Data.temperature,
            receivedPacket.sensorData.sht31Data.humidity,
            receivedPacket.sensorData.mq3_value,
            receivedPacket.sensorData.mq135_value,
            receivedPacket.sensorData.mq136_value,
            receivedPacket.sensorData.mq137_value
        );
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for serial to connect

  // Create the data queue
  dataQueue = xQueueCreate(DATA_QUEUE_LENGTH, sizeof(DataPacket));

  if (dataQueue == NULL) {
    Serial.println("Error creating the data queue");
    while (1);
  }

  // Create Logger Task on Core 0
  xTaskCreatePinnedToCore(
      loggerTask, "LoggerTask", 4096, NULL, 1, &loggerTaskHandle, 0
  );

  // Create Sensor Reader Task on Core 0
  xTaskCreatePinnedToCore(
      sensorReaderTask,
      "SensorReaderTask",
      4096,
      NULL,
      1,
      &sensorReaderTaskHandle,
      0
  );

  // Create Controller Task on Core 1
  xTaskCreatePinnedToCore(
      controllerTask,
      "ControllerTask",
      10000,
      NULL,
      2,  // Higher priority for the controller
      &controllerTaskHandle,
      1
  );
}

void loop() {
  vTaskDelete(NULL);  // The main loop is not used
}
