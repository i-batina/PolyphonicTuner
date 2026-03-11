#include "ADCDriver.h"

void ADCDriver::begin() {
  pinMode(Pin_CONVST, OUTPUT);
  pinMode(Pin_RD, OUTPUT);
  pinMode(Pin_CS, OUTPUT);
  pinMode(Pin_RESET, OUTPUT);
  pinMode(Pin_BUSY, INPUT);

  for (int i = 0; i < 16; i++) pinMode(DBUS[i], INPUT);

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

constexpr int ADCDriver::DBUS[16];
