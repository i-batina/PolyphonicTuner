#include "MotorControl.h"
#include <cmath>
#include <Arduino.h>

void MotorControl::setup() {
  for (int ch = 0; ch < NUM_MOTORS; ch++) {
    pinMode(MOTOR_PINS[ch][0], OUTPUT);
    pinMode(MOTOR_PINS[ch][1], OUTPUT);
    analogWriteFrequency(MOTOR_PINS[ch][0], PWM_FREQ);
    analogWriteFrequency(MOTOR_PINS[ch][1], PWM_FREQ);
  }
  stopAllMotors();
}

void MotorControl::setMotorSpeed(int motorIndex, int speed) {
  analogWrite(MOTOR_PINS[0][motorIndex], speed);
}

void MotorControl::stopAllMotors() {
  // Write zero to both pins of every channel so the motor that was running
  // actually stops, regardless of which channel was last driven.
  for (int ch = 0; ch < NUM_MOTORS; ch++) {
    analogWrite(MOTOR_PINS[ch][0], 0);
    analogWrite(MOTOR_PINS[ch][1], 0);
  }
}

void MotorControl::driveRaw(bool tuneUp, int speed, int channel) {
  speed   = constrain(speed, 0, MAX_SPEED);
  channel = constrain(channel, 0, NUM_MOTORS - 1);
  if (tuneUp) {
    analogWrite(MOTOR_PINS[channel][0], 0);
    analogWrite(MOTOR_PINS[channel][1], speed);
  } else {
    analogWrite(MOTOR_PINS[channel][0], speed);
    analogWrite(MOTOR_PINS[channel][1], 0);
  }
}

void MotorControl::tuneUp(float targetFreq, float currentFreq) { tune(targetFreq, currentFreq); }

void MotorControl::tuneDown(float targetFreq, float currentFreq) { tune(targetFreq, currentFreq); }

void MotorControl::tune(float targetFreq, float currentFreq, int channel) {
  // Cents error: positive = sharp (too high), negative = flat (too low)
  float cents    = 1200.0f * log2f(currentFreq / targetFreq);
  float absCents = fabsf(cents);

  if (absCents <= TUNE_THRESHOLD) {
    stopAllMotors();
    return;
  }

  bool tuningUp = cents < 0.0f;  // flat, so tighten
  channel       = constrain(channel, 0, NUM_MOTORS - 1);

  // Backlash compensation. On dirn reversal, fire full speed pulse to take up mechanical slack
  if (_hasLastDir[channel] && tuningUp != _lastTuneUp[channel]) {
    driveRaw(tuningUp, MAX_SPEED, channel);
    delay(BACKLASH_PULSE_MS);
    stopAllMotors();
  }
  _lastTuneUp[channel] = tuningUp;
  _hasLastDir[channel] = true;

  // Speed: full beyond CENTS_FULL_SPEED, linear ramp down to MIN_SPEED at TUNE_THRESHOLD
  int speed;
  if (absCents >= CENTS_FULL_SPEED) {
    speed = MAX_SPEED;
  } else {
    float t = (absCents - TUNE_THRESHOLD) / (CENTS_FULL_SPEED - TUNE_THRESHOLD);
    speed   = MIN_SPEED + (int)(t * (MAX_SPEED - MIN_SPEED));
  }

  if (cents > 0.0f) {
    // Sharp: tune down (loosen string)
    analogWrite(MOTOR_PINS[channel][0], speed);
    analogWrite(MOTOR_PINS[channel][1], 0);
  } else {
    // Flat: tune up (tighten string)
    analogWrite(MOTOR_PINS[channel][0], 0);
    analogWrite(MOTOR_PINS[channel][1], speed);
  }
}

bool MotorControl::isTargetReached(float targetFreq, float currentFreq, float centsOffTarget) {
  // Consider target reached if within a few cent (approx 0.04% frequency difference)
  // must have at least 5 consecutive readings within this range to be considered stable
  static int stableCount = 0;
  if (centsOffTarget <= TUNE_THRESHOLD) {
    stableCount++;
    if (stableCount >= 5) {
      stableCount = 0;  // reset for next time
      return true;
    }
  } else {
    stableCount = 0;
  }
  return false;
}