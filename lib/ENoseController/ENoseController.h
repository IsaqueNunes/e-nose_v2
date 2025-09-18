#ifndef E_NOSE_CONTROLLER_H
#define E_NOSE_CONTROLLER_H

#include <Arduino.h>
#include <LTC2310.h>
#include <Multiplexer.h>
#include <WaveGenerator.h>

#include <vector>

/**
 * @class ENoseController
 * @brief Manages the high-speed data acquisition and processing from the ADC
 * using a histogram-based approach to calculate a robust RMS value.
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
   * @brief Performs a high-speed sampling run for a fixed duration, builds a
   * histogram of the ADC values, and calculates a robust RMS value from it.
   * @param frequencyIndex The index of the frequency to use for the
   * measurement.
   * @param channel The multiplexer channel to use (1-based index).
   * @param durationMs The duration of the sampling period in milliseconds.
   * @return The calculated robust RMS value in ADC units.
   */
  float measureRmsForDuration(int frequencyIndex, int channel, int durationMs);

 private:
  WaveGenerator& waveGenerator;
  Multiplexer& multiplexer;
  LTC2310& adc;
  int waveSettlingTimeUs;

 private:
  // ... (outras variáveis)

  // --- CONSTANTES CORRIGIDAS PARA O LTC2310-14 ---
  // O valor bruto de 16 bits vai de 0 a 65535.
  static const int ADC_MAX_VALUE = 65535;
  static const int HISTOGRAM_BINS = 256;
  static const int ADC_VALUES_PER_BIN = (ADC_MAX_VALUE + 1) / HISTOGRAM_BINS;

  // Histogram array
  uint32_t adc_histogram[HISTOGRAM_BINS];
};

#endif  // E_NOSE_CONTROLLER_H
