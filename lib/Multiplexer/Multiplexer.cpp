#include "Multiplexer.h"

Multiplexer::Multiplexer(std::initializer_list<int> pins)
    : pins(pins), enabledChannelIndex(NO_CHANNEL_ENABLED) { }

void Multiplexer::init() {
  for (const int& pin : pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void Multiplexer::enableChannel(int channel) {
  if (!channelIsValid(channel)) {
    return;
  }

  // Disable the currently enabled channel, if any
  if (enabledChannelIndex != NO_CHANNEL_ENABLED) {
    digitalWrite(pins[enabledChannelIndex], LOW);
  }

  // Enable the new channel
  enabledChannelIndex = channel - 1;
  digitalWrite(pins[enabledChannelIndex], HIGH);
}

size_t Multiplexer::getChannelCount() const { return pins.size(); }

bool Multiplexer::channelIsValid(int channel) const {
  if (channel < 1 || channel > pins.size()) {
    Serial.printf(
        "ERROR: Invalid channel %d. Valid channels are 1 to %zu.\n",
        channel,
        pins.size()
    );
    return false;
  }
  return true;
}
