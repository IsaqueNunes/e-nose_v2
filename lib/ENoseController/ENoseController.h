#ifndef E_NOSE_CONTROLLER_H
#define E_NOSE_CONTROLLER_H

#include <Arduino.h>
#include <LTC2310.h>
#include <Multiplexer.h>
#include <WaveGenerator.h>

#include <vector>

#include "SensorData.h"  // Incluído para a struct LockInResult

class ENoseController {
 public:
  ENoseController(
      WaveGenerator& waveGenerator,
      Multiplexer& multiplexer,
      LTC2310& adc,
      int waveSettlingTimeUs
  );

  void init();

  /**
   * @brief Executa uma medição completa usando a técnica de lock-in amplifier.
   *
   * Realiza N medições de amplitude para uma dada frequência e canal,
   * e retorna a média e o desvio padrão desses resultados.
   *
   * @param frequencyHz A frequência a ser gerada e medida.
   * @param channel O canal do multiplexer a ser ativado.
   * @param num_readings O número de leituras de amplitude a serem feitas para a
   * estatística.
   * @param samples_per_reading O número de amostras do ADC por leitura de
   * amplitude.
   * @return Um objeto LockInResult contendo a média e o desvio padrão.
   */
  LockInResult performLockInMeasurement(
      long frequencyHz, int channel, int num_readings, int samples_per_reading
  );

 private:
  WaveGenerator& waveGenerator;
  Multiplexer& multiplexer;
  LTC2310& adc;
  int waveSettlingTimeUs;
};

#endif  // E_NOSE_CONTROLLER_H
