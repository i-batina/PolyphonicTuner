#include "MotorLogic.h"

// Motor 0 = Low E  (PWM=2, DIR=3, target=82.41 Hz)
// Motor 1 = A      (PWM=4, DIR=5, target=110.00 Hz)
const MotorLogic::Config MotorLogic::CONFIGS[MotorLogic::NUM_MOTORS] = {
    {2, 3, 82.41f, 0.5f},
    {4, 5, 110.0f, 0.5f},
};

void MotorLogic::begin() {
  for (int i = 0; i < NUM_MOTORS; i++) {
    pinMode(CONFIGS[i].pwmPin, OUTPUT);
    pinMode(CONFIGS[i].dirPin, OUTPUT);
    setSpeed(i, 0);
  }
}

bool MotorLogic::update(int motor, float measuredHz) {
  if (motor < 0 || motor >= NUM_MOTORS) return false;

  // String is silent — stop motor, no feedback
  if (measuredHz < SILENCE_HZ) {
    setSpeed(motor, 0);
    return false;
  }

  float error = CONFIGS[motor].targetHz - measuredHz;

  // Within tolerance — stop motor, in tune
  if (fabsf(error) < CONFIGS[motor].toleranceHz) {
    setSpeed(motor, 0);
    return true;
  }

  // P-controller: positive error → tighten (raise pitch), negative → loosen
  int speed = (int)(KP * error);
  if (speed > 255) speed = 255;
  if (speed < -255) speed = -255;

  setSpeed(motor, speed);
  return false;
}

void MotorLogic::stopAll() {
  for (int i = 0; i < NUM_MOTORS; i++) {
    setSpeed(i, 0);
  }
}

void MotorLogic::setSpeed(int motor, int speed) {
  uint8_t pwmPin = CONFIGS[motor].pwmPin;
  uint8_t dirPin = CONFIGS[motor].dirPin;

  if (speed == 0) {
    analogWrite(pwmPin, 0);
    digitalWrite(dirPin, LOW);
    return;
  }

  // HIGH = tighten (increase tension → raise pitch)
  digitalWrite(dirPin, speed > 0 ? HIGH : LOW);
  analogWrite(pwmPin, abs(speed));
}