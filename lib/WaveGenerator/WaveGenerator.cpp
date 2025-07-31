#include "WaveGenerator.h"

WaveGenerator::WaveGenerator(
    std::initializer_list<int> frequenciesHz,
    int waveDurationMs,
    int dataPin,
    int clockPin,
    int frameSyncPin
)
    : frequenciesHz(frequenciesHz),
      waveDurationMs(waveDurationMs),
      ad9833(dataPin, clockPin, frameSyncPin) {
  ad9833.begin();
  ad9833.setMode(MD_AD9833::MODE_SINE);
  lastWaveChangeTime = millis();
}

bool WaveGenerator::enoughFrequencyTimePassed() {
  return (millis() - lastWaveChangeTime) >= waveDurationMs;
}

void WaveGenerator::update() {
  if (!enoughFrequencyTimePassed()) return;

  lastWaveChangeTime = millis();

  // Increment the index and use modulo to cycle it
  currentFrequencyIndex = (currentFrequencyIndex + 1) % frequenciesHz.size();

  int frequency = frequenciesHz[currentFrequencyIndex];
  ad9833.setFrequency(MD_AD9833::CHAN_0, frequency);
}

int WaveGenerator::getCurrentFrequencyIndex() const {
  return currentFrequencyIndex;
}
