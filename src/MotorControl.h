

class MotorControl {
 public:
  void setup();

  void setMotorSpeed(int motorIndex, int speed);

  void stopAllMotors();

  void tuneUp(float targetFreq, float currentFreq);

  void tuneDown(float targetFreq, float currentFreq);

  bool isTargetReached(float targetFreq, float currentFreq);

 private:
  // Motor driver pins: 2 per channel for direction fwd or bkwrd
  /*
  static constexpr int MOT_D1[4] = {28, 29, 24, 25};
  static constexpr int MOT_D2[4] = {36, 33, 13, 37};
  */

  // 14, 15 -> CH2 (Low E)
  // 22, 23 -> CH1 (A)
  static constexpr int MOT_D3[4] = {14, 15, 22, 23};

  static constexpr int NUM_MOTORS = 2;
};