#include "ENoseController.h"

#include <cmath>  // NOVO: Necessário para a função sqrt()

ENoseController::ENoseController(
    WaveGenerator& waveGenerator,
    Multiplexer& multiplexer,
    LTC2310& adc,
    int waveSettlingTimeUs
)
    : waveGenerator(waveGenerator),
      multiplexer(multiplexer),
      adc(adc),
      waveSettlingTimeUs(waveSettlingTimeUs) { }

void ENoseController::init() {
  waveGenerator.init();
  multiplexer.init();
  adc.init();
}

// ALTERADO: Implementação completamente nova usando estatística para
// calcular o RMS verdadeiro.
float ENoseController::measureRmsForDuration(
    int frequencyIndex, int channel, int durationMs
) {
  // 1. Configura as condições e aguarda o assentamento
  waveGenerator.setFrequencyByIndex(frequencyIndex);
  multiplexer.enableChannel(channel);
  delayMicroseconds(waveSettlingTimeUs);

  // 2. Inicializa os acumuladores para o cálculo
  // Usamos 'double' para evitar overflow e perda de precisão com muitas
  // amostras.
  double sum_x = 0.0;
  double sum_x2 = 0.0;
  long count = 0;

  unsigned long startTime = millis();
  while (millis() - startTime < durationMs) {
    uint16_t rawValue = adc.readValue();

    const float V_REF = 2.5;
    // int16_t signedValue = (int16_t) rawValue >> 1;
    // float voltage = (signedValue / 16384.0f) * V_REF;

    // // Atualiza os acumuladores
    // sum_x += voltage;
    // sum_x2 += voltage * voltage;
    int16_t signedValue = (int16_t) rawValue >> 1;

    sum_x += signedValue;
    sum_x2 += signedValue * signedValue;
    count++;
  }

  if (count == 0) {
    return 0.0f;  // Evita divisão por zero
  }

  // 3. Calcula a média (offset DC) e a média dos quadrados
  double mean = sum_x / count;
  double mean_sq = sum_x2 / count;

  // 4. Calcula a variância (que é o quadrado do RMS da componente AC)
  double variance = mean_sq - (mean * mean);

  // Garante que a variância não seja negativa devido a erros de ponto flutuante
  if (variance < 0) {
    variance = 0;
  }

  // 5. O RMS da componente AC é a raiz quadrada da variância (desvio padrão).
  float v_rms = sqrt(variance);

  return v_rms;
}
