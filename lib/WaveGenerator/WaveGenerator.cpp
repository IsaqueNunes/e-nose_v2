#include "WaveGenerator.h"

WaveGenerator::WaveGenerator(
    std::initializer_list<int> frequenciesHz,
    int dataPin,
    int clockPin,
    int frameSyncPin
)
    : frequenciesHz(frequenciesHz), ad9833(dataPin, clockPin, frameSyncPin) {
  ad9833.begin();
  ad9833.setMode(MD_AD9833::MODE_SINE);
}

void WaveGenerator::setFrequencyByIndex(int index) {
  if (frequencyIndexIsValid(index)) {
    Serial.printf("ERROR: Invalid frequency index %d.\n", index);
    return;
  }
  int frequency = frequenciesHz[index];
  ad9833.setFrequency(MD_AD9833::CHAN_0, frequency);
}

size_t WaveGenerator::getFrequencyCount() const { return frequenciesHz.size(); }

bool WaveGenerator::frequencyIndexIsValid(int index) const {
  return index >= 0 && index < frequenciesHz.size();
}
