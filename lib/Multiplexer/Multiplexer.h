#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include <Arduino.h>

#include <initializer_list>
#include <vector>

/**
 * @brief Manages a set of digital output pins as a multiplexer.
 */
class Multiplexer {
 public:
  /**
   * @brief Construct a new Multiplexer object.
   * @param pins An initializer list of GPIO pin numbers for the channels.
   */
  Multiplexer(std::initializer_list<int> pins);

  /**
   * @brief Initializes GPIO pins.
   */
  void init();

  /**
   * @brief Enables a specific channel, disabling any previously enabled one.
   * @param channel The channel number to enable (1-based index).
   */
  void enableChannel(int channel);

  /**
   * @brief Gets the total number of channels.
   * @return The number of channels.
   */
  size_t getChannelCount() const;

 private:
  std::vector<int> pins;
  int enabledChannelIndex;
  static const int NO_CHANNEL_ENABLED = -1;

  /**
   * @brief Checks if the given channel number is valid.
   * @param channel The channel number to check (1-based index).
   * @return true if the channel is valid, false otherwise.
   */
  bool channelIsValid(int channel) const;
};

#endif  // MULTIPLEXER_H
