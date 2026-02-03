#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include <Arduino.h>

// Control pins
static constexpr int Pin_CONVST = 34; // falling edge starts conversion
static constexpr int Pin_BUSY = 35;   // BUSY high during conversion
static constexpr int Pin_RD = 36;     // active low
static constexpr int Pin_CS = 32;     // active low
static constexpr int Pin_RESET = 33;  // active high

// Parallel bus D0..D15
static constexpr int DBUS[16] = {37, 38, 39, 40, 41, 13, 14, 15,
                                 16, 17, 18, 19, 20, 21, 22, 23};

// Timing constants in us
static constexpr uint32_t CONVST_LOW_US = 2;
static constexpr uint32_t BUSY_TIMEOUT_US = 20000;
static constexpr uint32_t RD_LOW_SETTLE_US = 1;
static constexpr uint32_t RD_HIGH_GAP_US = 1;

class ADCDriver {
public:
  void begin() {
    pinMode(Pin_CONVST, OUTPUT);
    pinMode(Pin_RD, OUTPUT);
    pinMode(Pin_CS, OUTPUT);
    pinMode(Pin_RESET, OUTPUT);
    pinMode(Pin_BUSY, INPUT);

    for (int i = 0; i < 16; i++)
      pinMode(DBUS[i], INPUT);

    digitalWriteFast(Pin_CS, HIGH);
    digitalWriteFast(Pin_RD, HIGH);
    digitalWriteFast(Pin_CONVST, HIGH);
    digitalWriteFast(Pin_RESET, LOW);

    // Reset pulse (active high)
    digitalWriteFast(Pin_RESET, HIGH);
    delayMicroseconds(10);
    digitalWriteFast(Pin_RESET, LOW);
    delay(10);
  }

  inline void startConversion() {
    digitalWriteFast(Pin_CONVST, LOW);
    delayMicroseconds(CONVST_LOW_US);
    digitalWriteFast(Pin_CONVST, HIGH);
  }

  inline bool waitBusy() {
    uint32_t t0 = micros();
    while (!digitalReadFast(Pin_BUSY)) {
      if ((micros() - t0) > BUSY_TIMEOUT_US)
        return false;
    }
    while (digitalReadFast(Pin_BUSY)) {
      if ((micros() - t0) > BUSY_TIMEOUT_US)
        return false;
    }
    return true;
  }

  // Read the Nth word after a conversion by issuing N RD strobes
  // channel is 1-based: channel=1 returns the first word
  inline int16_t readChannel(uint8_t channel) {
    if (channel < 1)
      channel = 1;

    digitalWriteFast(Pin_CS, LOW);
    delayMicroseconds(1);

    uint16_t word = 0;
    for (uint8_t i = 1; i <= channel; i++) {
      digitalWriteFast(Pin_RD, LOW);
      delayMicroseconds(RD_LOW_SETTLE_US);

      word = 0;
      for (int bit = 0; bit < 16; bit++) {
        if (digitalReadFast(DBUS[bit]))
          word |= (1u << bit);
      }

      digitalWriteFast(Pin_RD, HIGH);
      delayMicroseconds(RD_HIGH_GAP_US);
    }

    digitalWriteFast(Pin_CS, HIGH);
    return (int16_t)word;
  }

  // Backwards-compatible helper
  inline int16_t readChannel1() { return readChannel(1); }
};

#endif
