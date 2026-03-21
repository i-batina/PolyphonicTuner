#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

class MotorControl {
 public:
  void setup();

  void setMotorSpeed(int motorIndex, int speed);

  void stopAllMotors();

  // Directly drives one motor channel in a given direction at the given speed (0-MAX_SPEED).
  // tuneUp=true increases pitch (tightens string); false decreases it.
  // channel: 0=Low E, 1=A, 2=D, 3=G, 4=B, 5=High E
  void driveRaw(bool tuneUp, int speed, int channel = 0);

  void tuneUp(float targetFreq, float currentFreq);

  void tuneDown(float targetFreq, float currentFreq);

  // Computes direction and speed from cents error, drives the given motor channel.
  // Full speed beyond CENTS_FULL_SPEED, linearly scaled down to MIN_SPEED
  // at TUNE_THRESHOLD, stopped inside TUNE_THRESHOLD.
  // channel: 0=Low E, 1=A, 2=D, 3=G, 4=B, 5=High E
  void tune(float targetFreq, float currentFreq, int channel = 0);

  bool isTargetReached(float targetFreq, float currentFreq, float centsOffTarget);

 private:
  // All 6 motor channels indexed [0]=Low E ... [5]=High E.
  // Each entry is {AIN1, AIN2} - the two PWM pins for that H-bridge half.
  static constexpr int MOTOR_PINS[6][2] = {
      {28, 29},  // [0] Low E  - D1_AIN
      {24, 25},  // [1] A      - D1_BIN
      {36, 33},  // [2] D      - D2_AIN
      {13, 37},  // [3] G      - D2_BIN
      {14, 15},  // [4] B      - D3_AIN
      {22, 23},  // [5] High E - D3_BIN
  };

  static constexpr int NUM_MOTORS       = 6;
  static constexpr int PWM_FREQ         = 20000;  // 20 kHz for quieter operation
  static constexpr int MAX_SPEED        = 250;    // Max PWM value
  static constexpr int TUNE_THRESHOLD   = 4;      // cents
  static constexpr int CENTS_FULL_SPEED = 40;     // cents error at which to apply full speed
  static constexpr int MIN_SPEED        = 170;    // Minimum speed to overcome motor static friction
};

#endif  // MOTOR_CONTROL_H