#include "Tuner.h"

void Tuner::removeDC() {
  int32_t sum = 0;
  for (int i = 0; i < BUFFER_SIZE; i++) sum += buffer[i];
  int16_t mean = (int16_t)(sum / BUFFER_SIZE);
  for (int i = 0; i < BUFFER_SIZE; i++) buffer[i] -= mean;
}

int16_t Tuner::peakToPeak() {
  int16_t minVal = 32767;
  int16_t maxVal = -32768;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    int16_t v = buffer[i];
    if (v < minVal) minVal = v;
    if (v > maxVal) maxVal = v;
  }
  return (int16_t)(maxVal - minVal);
}

void Tuner::addSample(int16_t sample) {
  if (!bufferReady) {
    buffer[writeIdx++] = sample;
    if (writeIdx >= BUFFER_SIZE) {
      bufferReady = true;
    }
  }
}

bool Tuner::isSilent(int16_t threshold) {
  int16_t minVal = 32767;
  int16_t maxVal = -32768;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    int16_t v = buffer[i];
    if (v < minVal) minVal = v;
    if (v > maxVal) maxVal = v;
  }
  return (int32_t)(maxVal - minVal) < threshold;
}

bool Tuner::isReady() { return bufferReady; }

void Tuner::reset() {
  writeIdx    = 0;
  bufferReady = false;
}

int16_t Tuner::getMaxAmplitude() {
  int16_t minVal = 32000;
  int16_t maxVal = -32000;
  for (int i = 0; i < BUFFER_SIZE; i++) {
    if (buffer[i] < minVal) minVal = buffer[i];
    if (buffer[i] > maxVal) maxVal = buffer[i];
  }
  return (maxVal - minVal);
}

float Tuner::detectPitch(float sampleRate) {
  // 1. Difference Function
  for (int tau = 0; tau < BUFFER_SIZE / 2; tau++) yinBuffer[tau] = 0;

  for (int tau = 1; tau < BUFFER_SIZE / 2; tau++) {
    for (int i = 0; i < BUFFER_SIZE / 2; i++) {
      float delta = buffer[i] - buffer[i + tau];
      yinBuffer[tau] += delta * delta;
    }
  }

  // CMND (Fixed division by zero)
  yinBuffer[0]     = 1;
  float runningSum = 0;
  for (int tau = 1; tau < BUFFER_SIZE / 2; tau++) {
    runningSum += yinBuffer[tau];
    if (runningSum < 0.0001) {  // Protect against zero division
      yinBuffer[tau] = 1;
    } else {
      yinBuffer[tau] *= tau;
      yinBuffer[tau] /= runningSum;
    }
  }

  // Absolute Threshold
  // tau=35 covers up to ~457 Hz (Hi E=329.63Hz -> tau~49, B=246.94Hz -> tau~65).
  // The frequency gate in processString filters any harmonic false positives.
  for (int tau = 35; tau < BUFFER_SIZE / 2; tau++) {
    if (yinBuffer[tau] < YIN_THRESHOLD) {
      // Descend to the true minimum of this dip before interpolating.
      // NOTE: first threshold crossing is the entry slope, not the bottom.
      // NOTE: Interpolating at entry point causes approx 25 cent scatter.
      while (tau + 1 < BUFFER_SIZE / 2 && yinBuffer[tau + 1] < yinBuffer[tau]) {
        tau++;
      }

      // Parabolic interpolation around the dip minimum
      float y1 = (tau > 0) ? yinBuffer[tau - 1] : yinBuffer[tau];
      float y2 = yinBuffer[tau];
      float y3 = (tau + 1 < BUFFER_SIZE / 2) ? yinBuffer[tau + 1] : yinBuffer[tau];

      float denom    = 2 * (2 * y2 - y3 - y1);
      float location = (denom != 0.0f) ? tau + (y3 - y1) / denom : tau;
      return sampleRate / location;
    }
  }

  // Fallback: global minimum in search range, with interpolation
  int   bestTau = -1;
  float bestVal = 100.0;

  for (int tau = 35; tau < BUFFER_SIZE / 2; tau++) {
    if (yinBuffer[tau] < bestVal) {
      bestVal = yinBuffer[tau];
      bestTau = tau;
    }
  }

  if (bestTau > 0 && bestVal < 0.4) {
    float y1 = yinBuffer[bestTau - 1];
    float y2 = yinBuffer[bestTau];
    float y3 = (bestTau + 1 < BUFFER_SIZE / 2) ? yinBuffer[bestTau + 1] : y2;

    float denom    = 2 * (2 * y2 - y3 - y1);
    float location = (denom != 0.0f) ? bestTau + (y3 - y1) / denom : bestTau;
    return sampleRate / location;
  }

  return 0.0;  // No pitch found
}