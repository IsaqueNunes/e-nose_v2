#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include <Arduino.h>
#include <WaveGenerator.h>

#include <initializer_list>
#include <vector>

class Multiplexer {
 private:
  static const int NO_CHANNEL_ENABLED = -1;
  std::vector<int> pins;
  WaveGenerator waveGenerator;

  int enabledChannelIndex;

  // Timing and frequency control logic now resides in this class
  int channelDurationMs;
  int currentFrequencyIndex;
  unsigned long lastChannelChangeTime;

  /**
   * @brief Checks if enough time has passed to switch to the next channel.
   * @return true if the duration has elapsed, false otherwise.
   */
  bool enoughChannelTimePassed() const;

 public:
  /**
   * @brief Construct a new Multiplexer object.
   * @param pins An initializer list of GPIO pin numbers for the channels.
   * @param frequenciesHz A list of frequencies for the wave generator.
   * @param channelDurationMs The duration each channel will be active for a
   * given frequency.
   */
  Multiplexer(
      std::initializer_list<int> pins,
      std::initializer_list<int> frequenciesHz,
      int channelDurationMs
  );

  /**
   * @brief Initializes GPIO pins, the wave generator, and state variables.
   */
  void init();

  /**
   * @brief Core update function to be called in the main loop.
   * Handles the logic of cycling through frequencies and, for each frequency,
   * cycling through all multiplexer channels.
   */
  void update();

  /**
   * @brief Enables a specific channel, disabling any previously enabled one.
   * @param channel The channel number to enable (1-based index).
   */
  void enableChannel(int channel);

  /**
   * @brief Disables a specific channel.
   * @param channel The channel number to disable (1-based index).
   */
  void disableChannel(int channel);

  /**
   * @brief Checks if the given channel number is valid.
   * @param channel The channel number to check (1-based index).
   * @return true if the channel is valid, false otherwise.
   */
  bool channelIsValid(int channel) const;

  /**
   * @brief Checks if the currently enabled channel is the specified one.
   * @param channel The channel number to check (1-based index).
   * @return true if the channel is already enabled, false otherwise.
   */
  bool channelIsEnabled(int channel) const;

  /**
   * @brief Checks if no channel is currently enabled.
   * @return true if no channel is enabled, false otherwise.
   */
  bool noChannelEnabled() const;
};

#endif  // MULTIPLEXER_H
