#ifndef WAVE_GENERATOR_H
#define WAVE_GENERATOR_H

#include <Arduino.h>
#include <MD_AD9833.h>
#include <SPI.h>

#include <initializer_list>
#include <vector>

class WaveGenerator {
 private:
  MD_AD9833 ad9833;

 public:
  const std::vector<int> frequenciesHz;

  /**
   * @brief Constructor for WaveGenerator.
   *
   * Initializes the AD9833 with default settings. Default pins are for ESP32
   * but can be overridden.
   *
   * @param frequenciesHz A list of frequencies for wave generation.
   * @param dataPin MOSI pin for SPI communication (default is 23).
   * @param clockPin SCK pin for SPI communication (default is 18).
   * @param frameSyncPin FSYNC pin for AD9833 (default is 5).
   */
  WaveGenerator(
      std::initializer_list<int> frequenciesHz,
      int dataPin = 23,
      int clockPin = 18,
      int frameSyncPin = 5
  );

  /**
   * @brief Sets the output frequency based on an index from the initial list.
   * @param index The index of the frequency to set.
   */
  void setFrequencyByIndex(int index);

  /**
   * @brief Gets the total number of frequencies available.
   * @return The count of frequencies.
   */
  size_t getFrequencyCount() const;

  bool frequencyIndexIsValid(int index) const;
};

#endif  // WAVE_GENERATOR_H
