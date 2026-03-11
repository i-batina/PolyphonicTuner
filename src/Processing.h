#ifndef PROCESSING_H
#define PROCESSING_H

#include <Arduino.h>
#include "Tuner.h"

class Processing {
 private:
  static constexpr float SAMPLE_RATE = 16000.0f;

  // Frequency gates per string wide enough to cover standard and common alternate tunings
  static constexpr float LOW_E_MIN_HZ = 50.0f;   // Covers down to Drop A (55 Hz)
  static constexpr float LOW_E_MAX_HZ = 90.0f;   // Covers up to open E (82.41 Hz) with headroom
  static constexpr float A_MIN_HZ     = 95.0f;   // Covers down to open-G 5th string (G2=98 Hz)
  static constexpr float A_MAX_HZ     = 135.0f;  // Covers up to open-E raised A (B2=123.5 Hz)

  static constexpr int MEDIAN_FRAMES = 5;
  static float         freqHistoryLowE[MEDIAN_FRAMES];
  static int           histCountLowE;
  static float         freqHistoryA[MEDIAN_FRAMES];
  static int           histCountA;

 public:
  static String getNoteName(float freq);

  // Calculate cents off
  static int getCentsOff(float freq);

  // Insertion-sort median over n values
  static float medianFreq(float* arr, int n);

  // Process one strings ready buffer, i.e. silence-check, pitch detect, median filter, print
  static void processString(
      Tuner& t, float minHz, float maxHz, float* history, int& histCount, const char* label);

  // ISR: one conversion captures all channels simultaneously on the ADC
  static void sampleISR();

  static void setup();

  static void loop();

 private:
  // Array of note names
  static constexpr const char* noteNames[12] = {
      "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
};

#endif  // PROCESSING_H