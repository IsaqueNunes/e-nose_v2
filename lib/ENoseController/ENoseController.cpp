#include "ENoseController.h"

ENoseController::ENoseController(
    WaveGenerator& waveGenerator,
    Multiplexer& multiplexer,
    LTC2310& adc,
    QueueHandle_t dataQueue,
    int channelDurationMs,
    int waveSettlingTimeUs
)
    : waveGenerator(waveGenerator),
      multiplexer(multiplexer),
      adc(adc),
      dataQueue(dataQueue),
      channelDurationMs(channelDurationMs),
      waveSettlingTimeUs(waveSettlingTimeUs),
      currentFrequencyIndex(0),
      currentChannel(0),
      lastChannelChangeTime(0) { }

void ENoseController::init() {
  waveGenerator.init();
  multiplexer.init();
  adc.init();

  waveGenerator.setFrequencyByIndex(currentFrequencyIndex);
  multiplexer.enableChannel(currentChannel + 1);
  lastChannelChangeTime = millis();
}

void ENoseController::update() {
  // Check if it's time to switch to the next channel
  if (millis() - lastChannelChangeTime >= channelDurationMs) {
    advanceToNextChannel();
    lastChannelChangeTime = millis();
  }

  // Perform ADC reading
  ADC_Data adcData;
  adcData.value = adc.readValue();
  adcData.channel = currentChannel + 1;
  adcData.frequency = waveGenerator.getFrequencyByIndex(currentFrequencyIndex);

  // Send data to the queue
  xQueueSend(dataQueue, &adcData, portMAX_DELAY);
}

void ENoseController::advanceToNextChannel() {
  currentChannel++;
  if (currentChannel >= multiplexer.getChannelCount()) {
    currentChannel = 0;
    advanceToNextFrequency();
  }
  multiplexer.enableChannel(currentChannel + 1);
}

void ENoseController::advanceToNextFrequency() {
  currentFrequencyIndex =
      (currentFrequencyIndex + 1) % waveGenerator.getFrequencyCount();
  waveGenerator.setFrequencyByIndex(currentFrequencyIndex);
  // Wait for the new frequency to settle
  delayMicroseconds(waveSettlingTimeUs);
}
