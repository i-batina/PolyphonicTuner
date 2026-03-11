#ifndef TUNER_H
#define TUNER_H

#include <Arduino.h>

#define BUFFER_SIZE 1024  // Power of 2 for YIN, min 1024 for low E accuracy
// NOTE: 2048 actually causes more scatter in low E
#define YIN_THRESHOLD 0.15

class Tuner {
 private:
  int16_t       buffer[BUFFER_SIZE];
  float         yinBuffer[BUFFER_SIZE / 2];
  volatile int  writeIdx    = 0;
  volatile bool bufferReady = false;

 public:
  String name;

  void removeDC();

  int16_t peakToPeak();

  Tuner(String stringName) : name(stringName) {
    memset(buffer, 0, sizeof(buffer));
    memset(yinBuffer, 0, sizeof(yinBuffer));
  }

  void addSample(int16_t sample);

  bool isSilent(int16_t threshold = 500);

  bool isReady();

  void reset();

  // Check amplitude to ignore silence
  int16_t getMaxAmplitude();

  // Calculates Hz from the buffer
  float detectPitch(float sampleRate);
};
#endif