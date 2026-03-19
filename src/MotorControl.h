#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

class MotorControl {
 public:
  void setup();

  void setMotorSpeed(int motorIndex, int speed);

  void stopAllMotors();

  void tuneUp(float targetFreq, float currentFreq);

  void tuneDown(float targetFreq, float currentFreq);

  bool isTargetReached(float targetFreq, float currentFreq);

 private:
  // Motor driver pins: 2 per motor channel for direction fwd or bkwrd

  // 14, 15 -> CH2
  static constexpr int MOT_D3_AIN[2] = {14, 15};

  static constexpr int NUM_MOTORS = 1;
};

#endif  // MOTOR_CONTROL_H