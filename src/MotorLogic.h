#ifndef MOTORLOGIC_H
#define MOTORLOGIC_H

#include <Arduino.h>

class MotorLogic {
 public:
  struct Config {
    uint8_t pwmPin;
    uint8_t dirPin;
    float   targetHz;
    float   toleranceHz;
  };

  static constexpr int   NUM_MOTORS = 2;
  static constexpr float KP         = 15.0f;  // PWM units per Hz of error
  static constexpr float SILENCE_HZ = 20.0f;  // Below this → treat as silent

  // Pin/target config per motor index (Low E = 0, A = 1)
  static const Config CONFIGS[NUM_MOTORS];

  // Setup GPIO pins. Call from setup().
  void begin();

  // P-controller update. Pass measuredHz = 0 when string is silent.
  // Computes error, applies PWM+DIR immediately, returns true if in tune.
  bool update(int motor, float measuredHz);

  // Immediately stop all motors.
  // Call before ADC sampling to prevent motor vibration from contaminating readings.
  void stopAll();

 private:
  // speed: -255 (full reverse) .. +255 (full forward), 0 = stop
  void setSpeed(int motor, int speed);
};

#endif  // MOTORLOGIC_H