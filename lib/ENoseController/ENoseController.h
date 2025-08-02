#ifndef E_NOSE_CONTROLLER_H
#define E_NOSE_CONTROLLER_H

#include <Arduino.h>
#include <LTC2310.h>
#include <Multiplexer.h>
#include <WaveGenerator.h>

#include "SensorData.h"

/**
 * @brief Manages the overall operation of the e-nose, coordinating the wave
 * generator, multiplexer, and ADC.
 */
class ENoseController {
 public:
  /**
   * @brief Construct a new ENoseController object.
   *
   * @param waveGenerator Reference to the WaveGenerator instance.
   * @param multiplexer Reference to the Multiplexer instance.
   * @param adc Reference to the LTC2310 ADC instance.
   * @param dataQueue Handle to the FreeRTOS queue for sending ADC data.
   * @param channelDurationMs Duration each multiplexer channel remains active
   * for a given frequency.
   * @param waveSettlingTimeUs Time to wait for the wave to settle after a
   * frequency change.
   */
  ENoseController(
      WaveGenerator& waveGenerator,
      Multiplexer& multiplexer,
      LTC2310& adc,
      QueueHandle_t dataQueue,
      int channelDurationMs,
      int waveSettlingTimeUs
  );

  /**
   * @brief Initializes the controller and its components.
   */
  void init();

  /**
   * @brief The main update loop for the controller.
   * This should be called repeatedly in its own FreeRTOS task.
   */
  void update();

 private:
  WaveGenerator& waveGenerator;
  Multiplexer& multiplexer;
  LTC2310& adc;
  QueueHandle_t dataQueue;

  int channelDurationMs;
  int waveSettlingTimeUs;
  int currentFrequencyIndex;
  int currentChannel;
  unsigned long lastChannelChangeTime;

  /**
   * @brief Switches to the next frequency in the list.
   */
  void advanceToNextFrequency();

  /**
   * @brief Switches to the next channel in the multiplexer.
   */
  void advanceToNextChannel();
};

#endif  // E_NOSE_CONTROLLER_H
