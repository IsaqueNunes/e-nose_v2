#include "LTC2310.h"

LTC2310::LTC2310(int csPin, SPIClass& spi) : csPin(csPin), spi(spi) { }

void LTC2310::init() {
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);  // O pino CNV deve estar em HIGH no estado inativo
  // A inicialização do SPI (`spi.begin()`) será feita no main.cpp para definir
  // os pinos corretos.
}

uint16_t LTC2310::readValue() {
  // O datasheet do LTC2310-14 (página 11, Timing Diagram) mostra que o clock
  // (SCK) fica em nível ALTO (HIGH) quando inativo, e os dados (SDO) são
  // capturados na borda de DESCIDA (falling edge). Isso corresponde ao
  // SPI_MODE3.
  spi.beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE3));

  // Passo 1: Inicia a conversão colocando o pino CNV (CS) em nível BAIXO.
  digitalWrite(csPin, LOW);

  // O tempo de conversão (t_CONV) é no máximo 220 ns. Um pequeno atraso é
  // suficiente.
  delayMicroseconds(1);

  // Passo 2: Lê os 16 bits de dados.
  uint16_t raw_data = spi.transfer16(0x0000);

  // Passo 3: Finaliza a transação e retorna CNV para nível ALTO.
  digitalWrite(csPin, HIGH);
  spi.endTransaction();

  return raw_data;
}

float LTC2310::toVoltage(uint16_t raw_value, float v_ref) {
  // O ADC retorna um valor de 15 bits em formato de complemento de dois.
  // A leitura de 16 bits é (B14, B13, ..., B0, 0).
  // Fazemos um shift para a direita para descartar o bit zero final e obter o
  // valor correto.
  int16_t signed_value = (int16_t) raw_value >> 1;

  // A faixa de conversão é de -16384 a +16383, correspondendo a -Vref a +Vref.
  float voltage = signed_value * (v_ref / 16384.0);

  return voltage;
}
