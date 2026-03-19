#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <Arduino.h>

class ADCDriver {
 public:
  // Sets up all GPIO directions and issues a reset pulse
  void begin();

  // Inlines, time-critical, must be visible to the compiler at every call site
  inline void startConversion() {
    digitalWriteFast(Pin_CONVST, LOW);
    delayMicroseconds(CONVST_LOW_US);
    digitalWriteFast(Pin_CONVST, HIGH);
  }

  inline bool waitBusy() {
    uint32_t t0 = micros();
    while (!digitalReadFast(Pin_BUSY)) {
      if ((micros() - t0) > BUSY_TIMEOUT_US) return false;
    }
    while (digitalReadFast(Pin_BUSY)) {
      if ((micros() - t0) > BUSY_TIMEOUT_US) return false;
    }
    return true;
  }

  // channel is 1-based: channel=1 returns the first word
  inline int16_t readChannel(uint8_t channel) {
    if (channel < 1) channel = 1;
    digitalWriteFast(Pin_CS, LOW);
    delayMicroseconds(1);
    uint16_t word = 0;
    for (uint8_t i = 1; i <= channel; i++) {
      digitalWriteFast(Pin_RD, LOW);
      delayMicroseconds(RD_LOW_SETTLE_US);
      word = 0;
      for (int bit = 0; bit < 16; bit++) {
        if (digitalReadFast(DBUS[bit])) word |= (1u << bit);
      }
      digitalWriteFast(Pin_RD, HIGH);
      delayMicroseconds(RD_HIGH_GAP_US);
    }
    digitalWriteFast(Pin_CS, HIGH);
    return (int16_t)word;
  }

  // Read count channels sequentially in a single CS assert
  inline void readChannels(int16_t* buf, uint8_t count) {
    if (count == 0) return;
    digitalWriteFast(Pin_CS, LOW);
    delayMicroseconds(1);
    for (uint8_t i = 0; i < count; i++) {
      digitalWriteFast(Pin_RD, LOW);
      delayMicroseconds(RD_LOW_SETTLE_US);
      uint16_t word = 0;
      for (int bit = 0; bit < 16; bit++) {
        if (digitalReadFast(DBUS[bit])) word |= (1u << bit);
      }
      buf[i] = (int16_t)word;
      digitalWriteFast(Pin_RD, HIGH);
      delayMicroseconds(RD_HIGH_GAP_US);
    }
    digitalWriteFast(Pin_CS, HIGH);
  }

  inline int16_t readChannel1() { return readChannel(1); }

 private:
  // Control pins
  static constexpr int Pin_CONVST = 32;  // falling edge starts conversion
  static constexpr int Pin_BUSY   = 34;  // BUSY high during conversion
  static constexpr int Pin_RD     = 35;  // active low
  static constexpr int Pin_CS     = 30;  // active low
  static constexpr int Pin_RESET  = 31;  // active high

  // Parallel bus D0...D15 NOTE: NEW TEENSY PINS
  static constexpr int DBUS[16] = {27, 38, 26, 0, 12, 1, 11, 2, 10, 3, 9, 4, 8, 5, 7, 6};

  // Timing constants in us
  static constexpr uint32_t CONVST_LOW_US    = 2;
  static constexpr uint32_t BUSY_TIMEOUT_US  = 20000;
  static constexpr uint32_t RD_LOW_SETTLE_US = 1;
  static constexpr uint32_t RD_HIGH_GAP_US   = 1;
};

#endif  // ADC_DRIVER_H