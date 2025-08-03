#include "ENoseController.h"

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

float ENoseController::measureRms(int frequencyIndex, int channel) {
  // 1. Set the conditions for the measurement
  waveGenerator.setFrequencyByIndex(frequencyIndex);
  multiplexer.enableChannel(channel);
  delayMicroseconds(waveSettlingTimeUs);  // Wait for everything to settle

  // 2. Perform high-speed sampling burst
  uint16_t samples[NUM_SAMPLES];
  for (int i = 0; i < NUM_SAMPLES; ++i) {
    samples[i] = adc.readValue();
    // A small delay might be needed if SPI is too fast, but usually readValue
    // is blocking enough
  }

  // 3. Sort the samples to find min and max robustly
  std::sort(samples, samples + NUM_SAMPLES);

  // 4. Get min and max, trimming outliers
  uint16_t min_robust = samples[TRIM_AMOUNT];
  uint16_t max_robust = samples[NUM_SAMPLES - 1 - TRIM_AMOUNT];

  // 5. Calculate RMS
  float v_pp = max_robust - min_robust;
  float v_peak = v_pp / 2.0;
  float v_rms = v_peak / sqrt(2.0);

  return v_rms;
}
