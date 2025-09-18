#include <Arduino.h>

#include "BLEManager.h"
#include "BME680_Sensor.h"
#include "ENoseController.h"
#include "LTC2310.h"
#include "Multiplexer.h"
#include "SHT31_Sensor.h"
#include "SensorData.h"
#include "WaveGenerator.h"

// --- Definições de Pinos ---
#define LTC2310_CS_PIN 4  // Pino CS (CNV) para o LTC2310
#define WAVEGEN_DATA_PIN 23
#define WAVEGEN_CLOCK_PIN 18
#define WAVEGEN_FSYNC_PIN 5
#define MQ3_PIN 33
#define MQ135_PIN 32
#define MQ136_PIN 34
#define MQ137_PIN 35

// --- Definições para o barramento HSPI (para o ADC) ---
#define LTC_HSPI_SCK_PIN 14
#define LTC_HSPI_MISO_PIN 19
#define LTC_HSPI_MOSI_PIN -1  // Não é utilizado pelo LTC2310

const int WAVE_SETTLING_TIME_US = 100;
const int RMS_MEASUREMENT_DURATION_S = 5;
const int CHANNEL_DURATION_S = 15;
const int NUM_RMS_MEASUREMENTS_PER_CHANNEL =
    CHANNEL_DURATION_S / RMS_MEASUREMENT_DURATION_S;

const std::initializer_list<long> FREQUENCIES_HZ = {1000, 5000, 10000, 20000};
const int NUM_MUX_CHANNELS = 4;

// Data Queue
#define DATA_QUEUE_LENGTH 50
QueueHandle_t dataQueue;

// --- Instâncias dos Objetos ---
// Cria um novo objeto SPI para a porta HSPI
SPIClass hspi(HSPI);

WaveGenerator waveGenerator(
    FREQUENCIES_HZ, WAVEGEN_DATA_PIN, WAVEGEN_CLOCK_PIN, WAVEGEN_FSYNC_PIN
);
Multiplexer multiplexer({27, 25, 26, 13});
// Passa a instância HSPI dedicada para o construtor do ADC
LTC2310 adc(LTC2310_CS_PIN, hspi);
BME680_Sensor bmeSensor;
SHT31_Sensor sht31Sensor;
ENoseController controller(
    waveGenerator, multiplexer, adc, WAVE_SETTLING_TIME_US
);
BLEManager bleManager("E-Nose");

// ... (restante do ficheiro main.cpp, incluindo as tarefas, permanece o mesmo)
// ... FreeRTOS Task Handles
TaskHandle_t sensorReaderTaskHandle;
TaskHandle_t dataTransferTaskHandle;

// Adicione no início do seu código
#define ADC_VREF 3.3
#define ADC_RESOLUTION 4095

float readMqSensorVoltage(int pin) {
  int rawValue = analogRead(pin);

  float pinVoltage = (float) rawValue / ADC_RESOLUTION * ADC_VREF;

  float sensorVoltage = pinVoltage;

  return sensorVoltage;
}

// Task for reading all sensors and performing measurements (Core 0)
void sensorReaderTask(void *pvParameters) {
  Serial.print("Sensor Reader Task running on core ");
  Serial.println(xPortGetCoreID());

  // Inicializa o barramento HSPI com os pinos corretos para o ADC
  hspi.begin(LTC_HSPI_SCK_PIN, LTC_HSPI_MISO_PIN, LTC_HSPI_MOSI_PIN, -1);

  controller.init();
  bmeSensor.init();
  sht31Sensor.init();

  pinMode(MQ3_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);
  pinMode(MQ136_PIN, INPUT);
  pinMode(MQ137_PIN, INPUT);

  for (;;) {  // Main loop, never exits
    // Loop through all frequencies
    for (int freq_idx = 0; freq_idx < FREQUENCIES_HZ.size(); ++freq_idx) {
      // Loop through all multiplexer channels
      for (int ch = 1; ch <= NUM_MUX_CHANNELS; ++ch) {
        Serial.printf(
            "Starting 15s cycle for Freq %ld Hz, Channel %d\n",
            waveGenerator.getFrequencyByIndex(freq_idx),
            ch
        );

        // Read slow sensors once per channel cycle
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

        float mq3_value = readMqSensorVoltage(MQ3_PIN);
        float mq135_value = readMqSensorVoltage(MQ135_PIN);
        float mq136_value = readMqSensorVoltage(MQ136_PIN);
        float mq137_value = readMqSensorVoltage(MQ137_PIN);

        packet.mq3_value = readMqSensorVoltage(MQ3_PIN);
        packet.mq135_value = readMqSensorVoltage(MQ135_PIN);
        packet.mq136_value = readMqSensorVoltage(MQ136_PIN);
        packet.mq137_value = readMqSensorVoltage(MQ137_PIN);

        Serial.println(mq3_value);
        Serial.println(mq135_value);
        Serial.println(mq136_value);
        Serial.println(mq137_value);

        // Perform 3 RMS measurements of 5s each
        for (int measurement_num = 1;
             measurement_num <= NUM_RMS_MEASUREMENTS_PER_CHANNEL;
             ++measurement_num) {
          Serial.printf(
              "-> Starting 5s RMS measurement #%d...\n", measurement_num
          );

          packet.adc_rms = controller.measureRmsForDuration(
              freq_idx, ch, RMS_MEASUREMENT_DURATION_S * 1000
          );
          packet.adc_channel = ch;
          packet.adc_frequency = waveGenerator.getFrequencyByIndex(freq_idx);

          Serial.printf("   -> RMS Calculated: %.2f\n", packet.adc_rms);

          // Send the complete data packet
          xQueueSend(dataQueue, &packet, portMAX_DELAY);
        }
      }
    }
  }
}

// Data Transfer Task (Core 1, remains the same)
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
  Serial.println("Starting E-Nose with new measurement logic...");

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
