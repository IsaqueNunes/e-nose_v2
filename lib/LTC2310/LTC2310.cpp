#include "LTC2310.h"

LTC2310::LTC2310(int csPin, SPIClass& spi) : csPin(csPin), spi(spi) { }

void LTC2310::init() {
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);
  // A inicialização do SPI (`spi.begin()`) será feita no main.cpp
}

uint16_t LTC2310::readValue() {
  // O datasheet (página 11) mostra que o clock (SCK) é inativo em HIGH,
  // e os dados (SDO) são capturados na borda de DESCIDA. Isso corresponde ao
  // SPI_MODE3.
  spi.beginTransaction(SPISettings(SPI_CLOCK, MSBFIRST, SPI_MODE3));

  // Passo 1: Inicia a conversão
  digitalWrite(csPin, LOW);

  // O tempo de conversão (t_CONV) é no máximo 220 ns.
  delayMicroseconds(1);

  // Passo 2: Lê os 16 bits de dados.
  uint16_t raw_data = spi.transfer16(0x0000);

  // Passo 3: Finaliza a transação
  digitalWrite(csPin, HIGH);
  spi.endTransaction();

  return raw_data;
}

// ALTERADO: A função agora usa o v_ref correto para o cálculo.
float LTC2310::toVoltage(uint16_t raw_value, float v_ref) {
  // O ADC retorna 15 bits (14 bits + sinal) em formato de complemento de dois,
  // seguido por um bit zero. O shift para a direita descarta o bit zero.
  int16_t signed_value = (int16_t) raw_value >> 1;

  // A faixa de conversão de -16384 a +16383 corresponde a -Vref a +Vref.
  // Usamos 16384.0f para o scaling.
  float voltage = signed_value * (v_ref / 16384.0f);

  return voltage;
}
