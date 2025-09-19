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
#define LTC2310_CS_PIN 4
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
#define LTC_HSPI_MOSI_PIN -1

// NOVO: Constantes para a nova estratégia de medição
const int WAVE_SETTLING_TIME_US = 100;
const int RMS_MEASUREMENT_DURATION_MS = 1000;  // Medir RMS por 1 segundo
const int SNAPSHOT_INTERVAL_MS =
    10000;  // Tirar um "snapshot" completo a cada 10 segundos

const std::initializer_list<long> FREQUENCIES_HZ = {100};
const int NUM_MUX_CHANNELS = 4;

// Data Queue
#define DATA_QUEUE_LENGTH 50
QueueHandle_t dataQueue;

// --- Instâncias dos Objetos ---
SPIClass hspi(HSPI);
WaveGenerator waveGenerator(
    FREQUENCIES_HZ, WAVEGEN_DATA_PIN, WAVEGEN_CLOCK_PIN, WAVEGEN_FSYNC_PIN
);
Multiplexer multiplexer({27});
LTC2310 adc(LTC2310_CS_PIN, hspi);
BME680_Sensor bmeSensor;
SHT31_Sensor sht31Sensor;
ENoseController controller(
    waveGenerator, multiplexer, adc, WAVE_SETTLING_TIME_US
);
BLEManager bleManager("E-Nose");

TaskHandle_t sensorReaderTaskHandle;
TaskHandle_t dataTransferTaskHandle;

#define MQ_ADC_VREF 2.5
#define ADC_RESOLUTION 4095

float readMqSensorVoltage(int pin) {
  int rawValue = analogRead(pin);
  float pinVoltage = (float) rawValue / ADC_RESOLUTION * MQ_ADC_VREF;
  return pinVoltage;
}

void sensorReaderTask(void *pvParameters) {
  Serial.print("Sensor Reader Task running on core ");
  Serial.println(xPortGetCoreID());

  hspi.begin(LTC_HSPI_SCK_PIN, LTC_HSPI_MISO_PIN, LTC_HSPI_MOSI_PIN, -1);
  controller.init();
  bmeSensor.init();
  sht31Sensor.init();
  pinMode(MQ3_PIN, INPUT);
  pinMode(MQ135_PIN, INPUT);
  pinMode(MQ136_PIN, INPUT);
  pinMode(MQ137_PIN, INPUT);

  // NOVO: Variáveis para controlar o ciclo de medições
  int current_freq_idx = 0;
  int current_channel = 1;

  for (;;) {  // Loop principal da tarefa
    unsigned long snapshotStartTime = millis();

    // 1. Ler todos os sensores comerciais (lentos) uma vez por snapshot
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
    packet.mq3_value = readMqSensorVoltage(MQ3_PIN);
    packet.mq135_value = readMqSensorVoltage(MQ135_PIN);
    packet.mq136_value = readMqSensorVoltage(MQ136_PIN);
    packet.mq137_value = readMqSensorVoltage(MQ137_PIN);

    // 2. Medir o RMS para a combinação atual de frequência e canal
    Serial.printf(
        "Measuring RMS for Freq %ld Hz, Channel %d...\n",
        waveGenerator.getFrequencyByIndex(current_freq_idx),
        current_channel
    );

    packet.adc_rms = controller.measureRmsForDuration(
        current_freq_idx, current_channel, RMS_MEASUREMENT_DURATION_MS
    );
    packet.adc_channel = current_channel;
    packet.adc_frequency = waveGenerator.getFrequencyByIndex(current_freq_idx);

    Serial.printf("   -> RMS Calculated: %.4f V\n", packet.adc_rms);

    // 3. Enviar o pacote de dados completo para a fila
    xQueueSend(dataQueue, &packet, portMAX_DELAY);

    // 4. Avançar para a próxima combinação de canal/frequência
    current_channel++;
    if (current_channel > NUM_MUX_CHANNELS) {
      current_channel = 1;
      current_freq_idx++;
      if (current_freq_idx >= waveGenerator.getFrequencyCount()) {
        current_freq_idx = 0;
      }
    }

    // 5. Aguardar o restante do tempo até o próximo snapshot
    unsigned long elapsedTime = millis() - snapshotStartTime;
    if (elapsedTime < SNAPSHOT_INTERVAL_MS) {
      vTaskDelay(pdMS_TO_TICKS(SNAPSHOT_INTERVAL_MS - elapsedTime));
    }
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
