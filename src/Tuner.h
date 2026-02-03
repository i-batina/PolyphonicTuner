#ifndef TUNER_H
#define TUNER_H

#include <Arduino.h>

#define BUFFER_SIZE 1024
#define YIN_THRESHOLD 0.15

class Tuner {
private:
  int16_t buffer[BUFFER_SIZE];
  float yinBuffer[BUFFER_SIZE / 2];
  volatile int writeIdx = 0;
  volatile bool bufferReady = false;

public:
  String name;

  void removeDC() {
    int32_t sum = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
      sum += buffer[i];
    int16_t mean = (int16_t)(sum / BUFFER_SIZE);
    for (int i = 0; i < BUFFER_SIZE; i++)
      buffer[i] -= mean;
  }

  int16_t peakToPeak() {
    int16_t minVal = 32767;
    int16_t maxVal = -32768;
    for (int i = 0; i < BUFFER_SIZE; i++) {
      int16_t v = buffer[i];
      if (v < minVal)
        minVal = v;
      if (v > maxVal)
        maxVal = v;
    }
    return (int16_t)(maxVal - minVal);
  }

  Tuner(String stringName) { name = stringName; }

  void addSample(int16_t sample) {
    if (!bufferReady) {
      buffer[writeIdx++] = sample;
      if (writeIdx >= BUFFER_SIZE) {
        bufferReady = true;
      }
    }
  }

  bool isSilent(int16_t threshold = 500) {
    int16_t minVal = 32767;
    int16_t maxVal = -32768;
    for (int i = 0; i < BUFFER_SIZE; i++) {
      int16_t v = buffer[i];
      if (v < minVal)
        minVal = v;
      if (v > maxVal)
        maxVal = v;
    }
    return (int32_t)(maxVal - minVal) < threshold;
  }

  bool isReady() { return bufferReady; }

  void reset() {
    writeIdx = 0;
    bufferReady = false;
  }

  // NEW: Check amplitude to ignore silence
  int16_t getMaxAmplitude() {
    int16_t minVal = 32000;
    int16_t maxVal = -32000;
    for (int i = 0; i < BUFFER_SIZE; i++) {
      if (buffer[i] < minVal)
        minVal = buffer[i];
      if (buffer[i] > maxVal)
        maxVal = buffer[i];
    }
    return (maxVal - minVal);
  }

  // Calculates Hz from the buffer
  float detectPitch(float sampleRate) {
    // 1. Difference Function (Squared Difference)
    for (int tau = 0; tau < BUFFER_SIZE / 2; tau++)
      yinBuffer[tau] = 0;

    for (int tau = 1; tau < BUFFER_SIZE / 2; tau++) {
      for (int i = 0; i < BUFFER_SIZE / 2; i++) {
        float delta = buffer[i] - buffer[i + tau];
        yinBuffer[tau] += delta * delta;
      }
    }

    // 2. Cumulative mean normalized difference (CMND)
    yinBuffer[0] = 1;
    float runningSum = 0;
    for (int tau = 1; tau < BUFFER_SIZE / 2; tau++) {
      runningSum += yinBuffer[tau];
      yinBuffer[tau] *= tau;
      yinBuffer[tau] /= runningSum;
    }

    // 3. Absolute threshold
    // Start search at tau=25 to ignore high freq harmonic noise (>660Hz)
    for (int tau = 25; tau < BUFFER_SIZE / 2; tau++) {
      if (yinBuffer[tau] < YIN_THRESHOLD) {
        // Found dip, then now interpolate for precision
        float y1 = (tau > 0) ? yinBuffer[tau - 1] : yinBuffer[tau];
        float y2 = yinBuffer[tau];
        float y3 =
            (tau + 1 < BUFFER_SIZE / 2) ? yinBuffer[tau + 1] : yinBuffer[tau];

        // Parabolic interpolation formula
        float location = tau + (y3 - y1) / (2 * (2 * y2 - y3 - y1));

        return sampleRate / location;
      }
    }
    return 0.0; // No pitch found
  }
};
#endif
