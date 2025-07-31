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

  /**
   * @brief Index of the currently enabled channel (0-based).
   * -1 means no channel is enabled.
   */
  int enabledChannelIndex;
  WaveGenerator waveGenerator;

 public:
  /**
   * @brief Construct a new Multiplexer object.
   * @param pins An initializer list of GPIO pin numbers to be used for the
   * channels.
   */
  Multiplexer(
      std::initializer_list<int> pins,
      std::initializer_list<int> frequenciesHz,
      int waveDurationMs
  );

  /**
   * @brief Initializes the GPIO pins for the multiplexer channels.
   * Sets all channel pins to OUTPUT and drives them LOW.
   */
  void init();

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
  bool channelIsValid(
      int channel
  ) const;  // Marked as const as it doesn't modify state

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

  void enableNextChannel();
  void update();
};

#endif  // MULTIPLEXER_H
