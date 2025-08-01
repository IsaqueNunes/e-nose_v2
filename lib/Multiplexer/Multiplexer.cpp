#include "Multiplexer.h"

Multiplexer::Multiplexer(
    std::initializer_list<int> pins,
    std::initializer_list<int> frequenciesHz,
    int channelDurationMs
)
    : pins(pins),
      waveGenerator(frequenciesHz),
      channelDurationMs(channelDurationMs) { }

void Multiplexer::init() {
  for (const int &pin : pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  enabledChannelIndex = NO_CHANNEL_ENABLED;
  currentFrequencyIndex = 0;
  lastChannelChangeTime = millis();

  waveGenerator.setFrequencyByIndex(currentFrequencyIndex);
  enableChannel(1);
}

bool Multiplexer::enoughChannelTimePassed() const {
  return (millis() - lastChannelChangeTime) >= channelDurationMs;
}

void Multiplexer::enableChannel(int channel) {
  if (!channelIsValid(channel)) return;

  if (!noChannelEnabled()) {
    digitalWrite(pins[enabledChannelIndex], LOW);
  }

  enabledChannelIndex = channel - 1;
  digitalWrite(pins[enabledChannelIndex], HIGH);
}

void Multiplexer::disableChannel(int channel) {
  if (!channelIsEnabled(channel)) {
    Serial.printf(
        "WARNING: Channel %d cannot be disabled; it is not enabled.\n", channel
    );
    return;
  }
  int channel_index_to_disable = channel - 1;
  digitalWrite(pins[channel_index_to_disable], LOW);
  enabledChannelIndex = NO_CHANNEL_ENABLED;
}

void Multiplexer::update() {
  if (!enoughChannelTimePassed()) {
    return;
  }
  lastChannelChangeTime = millis();

  int nextChannelIndex =
      (enabledChannelIndex == NO_CHANNEL_ENABLED) ? 0 : enabledChannelIndex + 1;

  if (nextChannelIndex >= pins.size()) {
    currentFrequencyIndex =
        (currentFrequencyIndex + 1) % waveGenerator.getFrequencyCount();

    waveGenerator.setFrequencyByIndex(currentFrequencyIndex);
    Serial.printf(
        "INFO: All channels cycled. New frequency index: %d\n",
        currentFrequencyIndex
    );

    nextChannelIndex = 0;
  }

  enableChannel(nextChannelIndex + 1);
  Serial.printf("INFO: Enabled channel %d\n", nextChannelIndex + 1);
}

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

bool Multiplexer::channelIsEnabled(int channel) const {
  if (!channelIsValid(channel)) {
    Serial.printf(
        "ERROR: Cannot check if channel %d is enabled; it is invalid.\n",
        channel
    );
    return false;
  }
  int channel_index = channel - 1;
  return enabledChannelIndex == channel_index;
}

bool Multiplexer::noChannelEnabled() const {
  return enabledChannelIndex == NO_CHANNEL_ENABLED;
}
