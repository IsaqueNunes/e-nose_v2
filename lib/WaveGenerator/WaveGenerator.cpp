#include "WaveGenerator.h"

WaveGenerator::WaveGenerator(
    std::initializer_list<long> frequenciesHz,
    int dataPin,
    int clockPin,
    int frameSyncPin
)
    : frequenciesHz(frequenciesHz),
      ad9833(dataPin, clockPin, frameSyncPin),
      activeChannel(MD_AD9833::CHAN_0) { }

void WaveGenerator::init() {
  ad9833.begin();
  ad9833.setMode(MD_AD9833::MODE_SINE);
}

void WaveGenerator::setFrequencyByIndex(int index) {
  if (!frequencyIndexIsValid(index)) {
    Serial.printf("ERROR: Invalid frequency index %d.\n", index);
    return;
  }

  long frequency = frequenciesHz[index];

  // Use the inactive channel to set the new frequency
  MD_AD9833::channel_t inactiveChannel = (activeChannel == MD_AD9833::CHAN_0)
                                             ? MD_AD9833::CHAN_1
                                             : MD_AD9833::CHAN_0;
  ad9833.setFrequency(inactiveChannel, frequency);
  Serial.printf(
      "WAVE GENERATOR: Setting frequency %ld Hz on channel %d.\n",
      frequency,
      inactiveChannel
  );

  // Switch to the new frequency
  ad9833.setActiveFrequency(inactiveChannel);
  Serial.printf("WAVE GENERATOR: Switched to channel %d.\n", inactiveChannel);
  activeChannel = inactiveChannel;
}

size_t WaveGenerator::getFrequencyCount() const { return frequenciesHz.size(); }

long WaveGenerator::getFrequencyByIndex(int index) const {
  if (!frequencyIndexIsValid(index)) {
    return -1;
  }
  return frequenciesHz[index];
}

bool WaveGenerator::frequencyIndexIsValid(int index) const {
  return index >= 0 && index < frequenciesHz.size();
}
