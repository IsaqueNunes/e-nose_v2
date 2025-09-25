#include <Arduino.h>

#include <vector>

#include "BLEManager.h"
#include "BME680_Sensor.h"
#include "ENoseController.h"
#include "LTC2310.h"
#include "Multiplexer.h"
#include "SHT31_Sensor.h"
#include "SensorData.h"  // Contém a nova DataPacket e definições
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

// --- Constantes para a nova estratégia de medição ---
const int WAVE_SETTLING_TIME_US =
    1000;                           // Aumentado para garantir estabilização
const int READINGS_PER_POINT = 20;  // N leituras para calcular média/std_dev
const int SAMPLES_PER_READING =
    1024;  // Amostras do ADC por leitura de amplitude
const int CYCLE_DELAY_MS =
    1000;  // Delay adicional ao final de um ciclo completo

// Lista de freq1024uências a serem varridas
const std::initializer_list<long> FREQUENCIES_HZ = {
    100, 1000, 5000, 10000, 50000, 100000
};
const int NUM_MUX_CHANNELS = 4;  // Definido em SensorData.h também

// Data Queue
#define DATA_QUEUE_LENGTH 5
QueueHandle_t dataQueue;

// --- Instâncias dos Objetos ---
SPIClass hspi(HSPI);
WaveGenerator waveGenerator(
    FREQUENCIES_HZ, WAVEGEN_DATA_PIN, WAVEGEN_CLOCK_PIN, WAVEGEN_FSYNC_PIN
);
Multiplexer multiplexer({27, 25, 26, 13});  // Exemplo com 4 canais
LTC2310 adc(LTC2310_CS_PIN, hspi);
BME680_Sensor bmeSensor;
SHT31_Sensor sht31Sensor;
ENoseController controller(
    waveGenerator, multiplexer, adc, WAVE_SETTLING_TIME_US
);
BLEManager bleManager("E-Nose_V2_LockIn");

TaskHandle_t sensorReaderTaskHandle;
TaskHandle_t dataTransferTaskHandle;

#define MQ_ADC_VREF 2.5  // Vref para os MQs
#define ADC_RESOLUTION 4095

float readMqSensorVoltage(int pin) {
  int rawValue = analogRead(pin);
  return (float) rawValue / ADC_RESOLUTION * MQ_ADC_VREF;
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

  for (;;) {  // Loop principal da tarefa (um ciclo completo por iteração)
    unsigned long cycleStartTime = millis();
    DataPacket packet;

    // 1. Ler todos os sensores comerciais (lentos) uma vez por ciclo
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

    // 2. Varre todas as frequências e canais para o sensor fabricado
    int data_index = 0;
    for (long freq : FREQUENCIES_HZ) {
      for (int ch = 1; ch <= NUM_MUX_CHANNELS; ++ch) {
        // channel settling time
        // delayMicroseconds(1000000);
        // delayMicroseconds(500000);
        // wait 5 seconds to channel stabilization
        vTaskDelay(pdMS_TO_TICKS(50));
        Serial.printf("Measuring Freq %ld Hz, Channel %d...\n", freq, ch);

        LockInResult result = controller.performLockInMeasurement(
            freq, ch, READINGS_PER_POINT, SAMPLES_PER_READING
        );

        if (data_index < ADC_DATA_POINTS) {
          packet.adc_mean[data_index] = result.mean;
          packet.adc_std_dev[data_index] = result.std_dev;
          data_index++;
        }
        Serial.printf(
            "   -> Mean: %.4f V, StdDev: %.4f V\n", result.mean, result.std_dev
        );
      }
    }

    // 3. Enviar o pacote de dados completo para a fila
    if (xQueueSend(dataQueue, &packet, pdMS_TO_TICKS(100)) != pdPASS) {
      Serial.println("WARN: Data queue is full!");
    }

    unsigned long cycleTime = millis() - cycleStartTime;
    Serial.printf("--- Cycle finished in %lu ms ---\n", cycleTime);

    // 4. Aguardar o restante do tempo até o próximo ciclo
    if (cycleTime < CYCLE_DELAY_MS) {
      vTaskDelay(pdMS_TO_TICKS(CYCLE_DELAY_MS - cycleTime));
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
  while (!Serial);  // Aguarda a conexão serial
  Serial.println("Starting E-Nose with Lock-In Amplifier logic...");

  bleManager.init();
  dataQueue = xQueueCreate(DATA_QUEUE_LENGTH, sizeof(DataPacket));

  if (dataQueue == NULL) {
    Serial.println("Error creating the data queue");
    while (1);
  }

  xTaskCreatePinnedToCore(
      sensorReaderTask,
      "SensorReaderTask",
      16384,  // Aumentado o stack para os cálculos
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
