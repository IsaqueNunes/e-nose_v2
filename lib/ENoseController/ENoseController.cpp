#include "ENoseController.h"

#include <cstring>  // For memset

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

float ENoseController::measureRmsForDuration(
    int frequencyIndex, int channel, int durationMs
) {
  // 1. Set conditions and wait for settling
  waveGenerator.setFrequencyByIndex(frequencyIndex);
  multiplexer.enableChannel(channel);
  delayMicroseconds(waveSettlingTimeUs);

  // 2. Clear previous histogram data and collect new data
  memset(adc_histogram, 0, sizeof(adc_histogram));
  uint32_t totalSamples = 0;

  unsigned long startTime = millis();
  while (millis() - startTime < durationMs) {
    uint16_t adcValue = adc.readValue();

    // Serial.println("ADC Value: " + String(adcValue));
    int bin = adcValue / ADC_VALUES_PER_BIN;
    if (bin >= HISTOGRAM_BINS) bin = HISTOGRAM_BINS - 1;
    adc_histogram[bin]++;
    totalSamples++;

    // --- A CORREÇÃO CRÍTICA ESTÁ AQUI ---
    // Cede o controle para o scheduler do FreeRTOS para evitar o watchdog
    // timeout. Uma pausa de 1ms é suficiente para manter o sistema responsivo.
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  if (totalSamples == 0) return 0.0f;

  // 3. Trim the histogram to find robust min and max
  uint32_t trimCount = totalSamples * 0.05;  // Trim 5% from each end
  uint32_t cumulativeCount = 0;
  int minBin = 0;
  for (int i = 0; i < HISTOGRAM_BINS; ++i) {
    cumulativeCount += adc_histogram[i];
    if (cumulativeCount >= trimCount) {
      minBin = i;
      break;
    }
  }

  cumulativeCount = 0;
  int maxBin = HISTOGRAM_BINS - 1;
  for (int i = HISTOGRAM_BINS - 1; i >= 0; --i) {
    cumulativeCount += adc_histogram[i];
    if (cumulativeCount >= trimCount) {
      maxBin = i;
      break;
    }
  }

  // Convert bin numbers back to approximate ADC values
  float min_robust = minBin * ADC_VALUES_PER_BIN;
  float max_robust = (maxBin + 1) * ADC_VALUES_PER_BIN - 1;

  // 4. Calculate RMS
  float v_pp_adc_units = max_robust - min_robust;

  // Converte a amplitude pico-a-pico de unidades do ADC para tensão
  // O valor de 15 bits com sinal cobre uma faixa de 32768 unidades para Vref
  float v_pp_volts =
      (v_pp_adc_units / 32768.0) * (3.3 / 2.0);  // Assumindo Vref = 3.3V

  float v_peak_volts = v_pp_volts / 2.0;
  float v_rms = v_peak_volts / sqrt(2.0);

  return v_rms;
}
