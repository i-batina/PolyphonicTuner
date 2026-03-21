#ifndef TUNER_H
#define TUNER_H

#include <Arduino.h>

#define BUFFER_SIZE 1024  // Power of 2 for YIN, min 1024 for low E accuracy
// NOTE: 2048 actually causes more scatter in low E
#define YIN_THRESHOLD 0.15

class Tuner {
 private:
  static constexpr float MPM_KEY_MAX_RATIO = 0.93f;  // McLeod pitch method key threshold

  int16_t       buffer[BUFFER_SIZE];
  float         yinBuffer[BUFFER_SIZE / 2];   // YIN CMND working buffer
  float         nsdfBuffer[BUFFER_SIZE / 2];  // MPM NSDF working buffer
  volatile int  writeIdx    = 0;
  volatile bool bufferReady = false;

 public:
  String  name;
  int16_t minP2PSinceReport = INT16_MAX;  // for onset detection

  void removeDC();

  int16_t peakToPeak();

  Tuner(String stringName) : name(stringName) {
    memset(buffer, 0, sizeof(buffer));
    memset(yinBuffer, 0, sizeof(yinBuffer));
    memset(nsdfBuffer, 0, sizeof(nsdfBuffer));
  }

  void addSample(int16_t sample);

  bool isSilent(int16_t threshold = 500);

  bool isReady();

  void reset();

  // Check amplitude to ignore silence
  int16_t getMaxAmplitude();

  // Calculates Hz from the buffer using YIN (default) or MPM (when USE_MPM is defined)
  float detectPitch(float sampleRate);

  // McLeod Pitch Method — always available regardless of USE_MPM build flag
  float detectPitchMPM(float sampleRate);
};
#endif