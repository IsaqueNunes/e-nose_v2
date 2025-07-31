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
  int waveDurationMs;
  int currentFrequencyIndex = 0;
  unsigned long lastWaveChangeTime = 0;

  /**
   * Check if enough time has passed since the last wave change.
   *
   * This method checks if the specified wave duration has elapsed since the
   * last wave change, allowing the next frequency to be set.
   *
   * @return true if enough time has passed, false otherwise.
   */
  bool enoughFrequencyTimePassed();

 public:
  const std::vector<int> frequenciesHz;

  /**
   * Constructor for WaveGenerator.
   *
   * Initializes the AD9833 with the specified frequencies and wave duration.
   * Default pins are set for ESP32, but can be overridden.
   *
   * @param frequenciesHz Frequencies in Hz for the wave generation.
   * @param waveDurationMs Duration of each wave in milliseconds.
   * @param dataPin MOSI pin for SPI communication (default is 23).
   * @param clockPin SCK pin for SPI communication (default is 18).
   * @param frameSyncPin FSYNC pin for AD9833 (default is 5).
   */
  WaveGenerator(
      std::initializer_list<int> frequenciesHz,
      int waveDurationMs,
      int dataPin = 23,
      int clockPin = 18,
      int frameSyncPin = 5
  );

  /**
   * Update the wave generator state.
   *
   * This method should be called regularly (e.g., in the loop() function) to
   * update the wave generator and change the frequency if enough time has
   * passed.
   */
  void update();

  /**
   * Get the current frequency index.
   *
   * This method returns the frequency currently set in the AD9833.
   *
   * @return The current frequency in Hz.
   */
  int getCurrentFrequencyIndex() const;
};

#endif  // WAVE_GENERATOR_H
