#include "Multiplexer.h"

Multiplexer::Multiplexer(
    std::initializer_list<int> pins,
    std::initializer_list<int> frequenciesHz,
    int waveDurationMs
)
    : pins(pins), waveGenerator(frequenciesHz, waveDurationMs) {
  init();
}

void Multiplexer::init() {
  for (const int &pin : pins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  enabledChannelIndex = NO_CHANNEL_ENABLED;  // Reset state on init
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
        "ERROR: Cannot check if channel %d is enabled because it is invalid.\n",
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
        "WARNING: Channel %d cannot be disabled because it is not enabled.\n",
        channel
    );
    return;
  }

  int channel_index_to_disable = channel - 1;

  digitalWrite(pins[channel_index_to_disable], LOW);
  enabledChannelIndex = NO_CHANNEL_ENABLED;
}

void Multiplexer::enableNextChannel() {
  if (noChannelEnabled()) {
    enableChannel(1);
    return;
  }

  int nextChannelIndex = (enabledChannelIndex + 1) % pins.size();
  enableChannel(nextChannelIndex + 1);  // Convert to 1-based index
}

void Multiplexer::update() {
  if (noChannelEnabled()) {
    Serial.println(
        "WARNING: No channel is enabled. Cannot update wave generator."
    );
    return;
  }

  if (waveGenerator.getCurrentFrequencyIndex() ==
      waveGenerator.frequenciesHz.size() - 1) {
    enableNextChannel();

    Serial.printf("INFO: Enabled next channel %d.\n", enabledChannelIndex + 1);
  }

  waveGenerator.update();
}
