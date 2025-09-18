#include "WaveGenerator.h"

WaveGenerator::WaveGenerator(
    std::initializer_list<long> frequenciesHz,
    int dataPin,
    int clockPin,
    int frameSyncPin
)
    : frequenciesHz(frequenciesHz),
      ad9833(dataPin, clockPin, frameSyncPin),
      activeChannel(MD_AD9833::CHAN_0)  // Começa com o canal 0
{ }

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

  // Determina qual canal está inativo para programar a nova frequência
  MD_AD9833::channel_t inactiveChannel = (activeChannel == MD_AD9833::CHAN_0)
                                             ? MD_AD9833::CHAN_1
                                             : MD_AD9833::CHAN_0;

  Serial.printf(
      "Setting Freq %ld Hz on inactive channel %d\n", frequency, inactiveChannel
  );

  // Define a frequência no canal que não está a ser usado
  ad9833.setFrequency(inactiveChannel, frequency);

  // Ativa o canal recém-programado (a comutação é instantânea)
  ad9833.setActiveFrequency(inactiveChannel);

  // Atualiza o estado para a próxima chamada
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
