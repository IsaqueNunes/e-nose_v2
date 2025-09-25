#include "ENoseController.h"

#include <cmath>  // Necessário para sin, cos, sqrt

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

LockInResult ENoseController::performLockInMeasurement(
    long frequencyHz, int channel, int num_readings, int samples_per_reading
) {
  // 1. Configura as condições e aguarda o assentamento
  waveGenerator.setFrequency(
      frequencyHz
  );  // Assume que a WaveGenerator tem um método setFrequency
  multiplexer.enableChannel(channel);
  delayMicroseconds(waveSettlingTimeUs);

  std::vector<float> amplitude_results;
  amplitude_results.reserve(num_readings);

  const float V_REF = 2.5f;  // Tensão de referência do ADC

  for (int i = 0; i < num_readings; ++i) {
    double sum_I = 0.0;
    double sum_Q = 0.0;
    unsigned long start_time_us = micros();

    for (int k = 0; k < samples_per_reading; ++k) {
      uint16_t raw_value = adc.readValue();
      // Converte o valor bruto para tensão. O ADC é diferencial.
      int16_t signed_value =
          (int16_t) raw_value >> 1;  // Descarta o último bit (zero)
      float voltage = (signed_value / 16384.0f) * V_REF;

      // Calcula o tempo atual em segundos para as ondas de referência
      float t = (micros() - start_time_us) / 1000000.0f;

      // Gera as referências e multiplica
      float ref_sin = sin(2.0 * PI * frequencyHz * t);
      float ref_cos = cos(2.0 * PI * frequencyHz * t);

      sum_I += voltage * ref_sin;
      sum_Q += voltage * ref_cos;
    }

    // Calcula a média dos componentes I e Q
    double mean_I = sum_I / samples_per_reading;
    double mean_Q = sum_Q / samples_per_reading;

    // Calcula a amplitude. O fator 2 é para normalizar a amplitude.
    float amplitude = 2.0 * sqrt(mean_I * mean_I + mean_Q * mean_Q);
    amplitude_results.push_back(amplitude);

    vTaskDelay(1);
  }

  // 2. Calcula a Média e o Desvio Padrão das amplitudes
  LockInResult result = {0.0f, 0.0f};
  if (amplitude_results.empty()) {
    return result;
  }

  double sum = 0.0;
  for (float val : amplitude_results) {
    sum += val;
  }
  result.mean = sum / amplitude_results.size();

  double sum_sq_diff = 0.0;
  for (float val : amplitude_results) {
    sum_sq_diff += (val - result.mean) * (val - result.mean);
  }
  // Usa a fórmula do desvio padrão da amostra (N-1) se N > 1
  if (amplitude_results.size() > 1) {
    result.std_dev = sqrt(sum_sq_diff / (amplitude_results.size() - 1));
  } else {
    result.std_dev = 0.0;
  }

  return result;
}
