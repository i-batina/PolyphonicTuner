#include "ADCDriver.h"
#include "Tuner.h"
#include <Arduino.h>
#include <IntervalTimer.h>

ADCDriver adc;
Tuner tuner("Monophonic");

IntervalTimer sampleTimer;

static constexpr float SAMPLE_RATE = 16000.0f;
static constexpr float LOW_E_MIN_HZ = 50.0f;
static constexpr float LOW_E_MAX_HZ = 90.0f;

// ISR: keep it short—no Serial prints here
void sampleISR() {
  adc.startConversion();

  if (!adc.waitBusy())
    return; // NOTE: level shifter bricks BUSY pin

  int16_t s = adc.readChannel(2); // Read low E string (channel 2 for now)
  tuner.addSample(s);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  // Flag if nothing prints
  if (CrashReport) {
    Serial.print(CrashReport); // Teensy 4.x feature [web:70]
  }

  Serial.println("STARTING PITCH DETECTOR...");

  adc.begin();

  // 16kHz => 62.5 us period
  sampleTimer.begin(sampleISR, 62.5);

  Serial.println("SAMPLING STARTED");
}

void loop() {
  if (!tuner.isReady())
    return;

  static bool zeroPitchAlreadyReported = false;

  sampleTimer.end(); // pause sampling during heavy YIN (prevents corruption)

  int16_t p2p = tuner.peakToPeak();
  tuner.removeDC();

  float f = 0.0f;
  if (p2p < 500) { // gate (tune this)
    f = 0.0f;
  } else {
    f = tuner.detectPitch(SAMPLE_RATE);
  }

  // Low E (currently channel 2): ignore anything outside expected range
  if (!(f >= LOW_E_MIN_HZ && f <= LOW_E_MAX_HZ)) {
    f = 0.0f;
  }

  const bool isZeroPitch = (f == 0.0f);
  const bool shouldPrint = (!isZeroPitch) || (!zeroPitchAlreadyReported);

  if (shouldPrint) {
    // Serial.print("P2P:");
    // Serial.print(p2p);
    Serial.print("  Pitch:");
    Serial.println(f, 2);
  }

  if (isZeroPitch) {
    zeroPitchAlreadyReported = true;
  } else {
    zeroPitchAlreadyReported = false;
  }

  tuner.reset();
  sampleTimer.begin(sampleISR, 62.5);
}
