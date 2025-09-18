#ifndef LTC2310_H
#define LTC2310_H

#include <Arduino.h>
#include <SPI.h>

class LTC2310 {
 public:
  /**
   * @brief Construtor da classe para o ADC LTC2310.
   * @param csPin O pino de Chip Select (CNV) para o ADC.
   * @param spi A instância da classe SPI a ser usada (e.g., VSPI ou HSPI).
   */
  LTC2310(int csPin, SPIClass& spi);

  /**
   * @brief Inicializa o pino CS e a comunicação SPI.
   */
  void init();

  /**
   * @brief Lê o valor bruto de 16 bits do ADC.
   * @return O valor lido (15 bits de dados + 1 bit zero).
   */
  uint16_t readValue();

  /**
   * @brief Converte o valor bruto lido para tensão.
   * @param raw_value O valor bruto de 16 bits vindo de readValue().
   * @param v_ref A tensão de referência usada pelo ADC.
   * @return A tensão calculada em Volts.
   */
  float toVoltage(uint16_t raw_value, float v_ref);

 private:
  int csPin;
  SPIClass& spi;
  // A velocidade do clock SPI pode ir até 64MHz, mas começamos com um valor
  // seguro.
  const int SPI_CLOCK = 20000000;  // 20 MHz
};

#endif  // LTC2310_H
