#ifndef WAVE_GENERATOR_H
#define WAVE_GENERATOR_H

#include <Arduino.h>
#include <MD_AD9833.h>
#include <SPI.h>

#include <initializer_list>
#include <vector>

/**
 * @brief Manages the AD9833 Waveform Generator.
 */
class WaveGenerator {
 public:
  /**
   * @brief Constructor for WaveGenerator.
   *
   * @param frequenciesHz A list of frequencies for wave generation.
   * @param dataPin MOSI pin for SPI communication.
   * @param clockPin SCK pin for SPI communication.
   * @param frameSyncPin FSYNC pin for AD9833.
   */
  WaveGenerator(
      std::initializer_list<long> frequenciesHz,
      int dataPin = 23,
      int clockPin = 18,
      int frameSyncPin = 5
  );

  /**
   * @brief Initializes the AD9833.
   */
  void init();

  /**
   * @brief Sets the output frequency based on an index from the initial list.
   * This method uses both frequency registers for a fast and smooth transition.
   * @param index The index of the frequency to set.
   */
  void setFrequencyByIndex(int index);

  /**
   * @brief Gets the total number of frequencies available.
   * @return The count of frequencies.
   */
  size_t getFrequencyCount() const;

  /**
   * @brief Gets a frequency by its index.
   * @param index The index of the frequency.
   * @return The frequency in Hz.
   */
  long getFrequencyByIndex(int index) const;

 private:
  MD_AD9833 ad9833;
  const std::vector<long> frequenciesHz;
  MD_AD9833::channel_t activeChannel;

  /**
   * @brief Checks if a frequency index is valid.
   * @param index The index to check.
   * @return true if the index is valid, false otherwise.
   */
  bool frequencyIndexIsValid(int index) const;
};

#endif  // WAVE_GENERATOR_H
