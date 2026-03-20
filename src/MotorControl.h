#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

class MotorControl {
 public:
  void setup();

  void setMotorSpeed(int motorIndex, int speed);

  void stopAllMotors();

  void tuneUp(float targetFreq, float currentFreq);

  void tuneDown(float targetFreq, float currentFreq);

  // Computes direction and speed from cents error, drives motor accordingly.
  // Full speed beyond CENTS_FULL_SPEED, linearly scaled down to MIN_SPEED
  // at TUNE_THRESHOLD, stopped inside TUNE_THRESHOLD.
  void tune(float targetFreq, float currentFreq);

  bool isTargetReached(float targetFreq, float currentFreq);

 private:
  // Motor driver pins: 2 per motor channel for direction fwd or bkwrd

  // 28, 29 -> CH2
  static constexpr int MOT_D3_AIN[2]    = {28, 29};
  static constexpr int NUM_MOTORS       = 1;
  static const int     PWM_FREQ         = 20000;  // 20 kHz for quieter operation
  static const int     MAX_SPEED        = 255;    // Max PWM value
  static const int     TUNE_THRESHOLD   = 7;      // cents
  static const int     CENTS_FULL_SPEED = 20;     // cents error at which to apply full speed
  static const int     MIN_SPEED        = 100;    // Minimum speed to overcome motor static friction
};

#endif  // MOTOR_CONTROL_H