#ifndef E_NOSE_CONTROLLER_H
#define E_NOSE_CONTROLLER_H

#include <Arduino.h>
#include <LTC2310.h>
#include <Multiplexer.h>
#include <WaveGenerator.h>

#include <algorithm>  // For std::sort
#include <vector>

/**
 * @brief Manages the high-speed data acquisition and processing from the ADC.
 */
class ENoseController {
 public:
  /**
   * @brief Construct a new ENoseController object.
   * @param waveGenerator Reference to the WaveGenerator instance.
   * @param multiplexer Reference to the Multiplexer instance.
   * @param adc Reference to the LTC2310 ADC instance.
   * @param waveSettlingTimeUs Time to wait for the wave to settle after a
   * frequency change.
   */
  ENoseController(
      WaveGenerator& waveGenerator,
      Multiplexer& multiplexer,
      LTC2310& adc,
      int waveSettlingTimeUs
  );

  /**
   * @brief Initializes the controller and its components.
   */
  void init();

  /**
   * @brief Performs a high-speed sampling run for a given frequency and
   * channel, and returns the calculated RMS value.
   * @param frequencyIndex The index of the frequency to use for the
   * measurement.
   * @param channel The multiplexer channel to use (1-based index).
   * @return The calculated RMS value in ADC units.
   */
  float measureRms(int frequencyIndex, int channel);

 private:
  WaveGenerator& waveGenerator;
  Multiplexer& multiplexer;
  LTC2310& adc;
  int waveSettlingTimeUs;

  static const int NUM_SAMPLES = 256;
  static const int TRIM_AMOUNT = 12;  // ~5% of NUM_SAMPLES
};

#endif  // E_NOSE_CONTROLLER_H
