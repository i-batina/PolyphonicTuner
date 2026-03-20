#include "MotorControl.h"
#include <cmath>
#include <Arduino.h>

void MotorControl::setup() {
  pinMode(MOT_D3_AIN[0], OUTPUT);
  pinMode(MOT_D3_AIN[1], OUTPUT);

  analogWriteFrequency(MOT_D3_AIN[0], 20000);  // Set PWM frequency to 20 kHz for quieter
  analogWriteFrequency(MOT_D3_AIN[1], 20000);

  stopAllMotors();
}

void MotorControl::setMotorSpeed(int motorIndex, int speed) {
  analogWrite(MOT_D3_AIN[motorIndex], speed);
}

void MotorControl::stopAllMotors() {
  for (int i = 0; i < NUM_MOTORS; i++) {
    setMotorSpeed(i, 0);
  }
}

void MotorControl::tuneUp(float targetFreq, float currentFreq) { tune(targetFreq, currentFreq); }

void MotorControl::tuneDown(float targetFreq, float currentFreq) { tune(targetFreq, currentFreq); }

void MotorControl::tune(float targetFreq, float currentFreq) {
  // Cents error: positive = sharp (too high), negative = flat (too low)
  float cents    = 1200.0f * log2f(currentFreq / targetFreq);
  float absCents = fabsf(cents);

  if (absCents <= TUNE_THRESHOLD) {
    stopAllMotors();
    return;
  }

  // Speed: full beyond CENTS_FULL_SPEED, linear ramp down to MIN_SPEED at TUNE_THRESHOLD
  int speed;
  if (absCents >= CENTS_FULL_SPEED) {
    speed = MAX_SPEED;
  } else {
    float t = (absCents - TUNE_THRESHOLD) / (CENTS_FULL_SPEED - TUNE_THRESHOLD);
    speed   = MIN_SPEED + (int)(t * (MAX_SPEED - MIN_SPEED));
  }

  // Positive cents = sharp = tune down (loosen string = one pin direction)
  // Negative cents = flat  = tune up   (tighten string = other pin direction)
  if (cents > 0.0f) {
    // Tune down: AIN[0]=0, AIN[1]=speed
    analogWrite(MOT_D3_AIN[0], 0);
    analogWrite(MOT_D3_AIN[1], speed);
  } else {
    // Tune up: AIN[0]=speed, AIN[1]=0
    analogWrite(MOT_D3_AIN[0], speed);
    analogWrite(MOT_D3_AIN[1], 0);
  }
}

bool MotorControl::isTargetReached(float targetFreq, float currentFreq) {
  // Consider target reached if within 7 cent (approx 0.06% frequency difference)
  // must have at least 5 consecutive readings within this range to be considered stable

  static int stableCount = 0;
  float      diff        = fabsf(currentFreq - targetFreq);
  if (diff / targetFreq < 0.0006f) {
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