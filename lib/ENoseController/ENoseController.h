#ifndef E_NOSE_CONTROLLER_H
#define E_NOSE_CONTROLLER_H

#include <Arduino.h>
#include <LTC2310.h>
#include <Multiplexer.h>
#include <WaveGenerator.h>

#include <vector>

class ENoseController {
 public:
  ENoseController(
      WaveGenerator& waveGenerator,
      Multiplexer& multiplexer,
      LTC2310& adc,
      int waveSettlingTimeUs
  );

  void init();

  // ALTERADO: A assinatura da função permanece a mesma, mas a implementação
  // mudará completamente.
  float measureRmsForDuration(int frequencyIndex, int channel, int durationMs);

 private:
  WaveGenerator& waveGenerator;
  Multiplexer& multiplexer;
  LTC2310& adc;
  int waveSettlingTimeUs;

  // ALTERADO: Removido o array de histograma e constantes associadas.
  // A nova abordagem não precisa armazenar amostras.
};

#endif  // E_NOSE_CONTROLLER_H
