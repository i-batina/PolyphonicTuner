#ifndef PROCESSING_H
#define PROCESSING_H

#include <Arduino.h>
#include "Tuner.h"

class Processing {
 private:
  static constexpr float SAMPLE_RATE = 16000.0f;

  // Frequency gates per string wide enough to cover standard and common alternate tunings
  static constexpr float LOW_E_MIN_HZ = 50.0f;   // down to Drop A (55 Hz)
  static constexpr float LOW_E_MAX_HZ = 90.0f;   // up to open E (82.41 Hz)
  static constexpr float A_MIN_HZ     = 95.0f;   // down to open-G 5th string (G2=98 Hz)
  static constexpr float A_MAX_HZ     = 135.0f;  // up to open-E raised A (B2=123.5 Hz)
  static constexpr float D_MIN_HZ     = 120.0f;  // down to open-D 4th string (D3=146.8 Hz)
  static constexpr float D_MAX_HZ     = 200.0f;  // up to open-D raised D (E3=164.8 Hz)
  static constexpr float G_MIN_HZ     = 150.0f;  // down to open-G 3rd string (G3=196.0 Hz)
  static constexpr float G_MAX_HZ     = 250.0f;  // up to open-G raised G (A3=220.0 Hz)
  static constexpr float B_MIN_HZ     = 200.0f;  // down to open-B 2nd string (B3=246.9 Hz)
  static constexpr float B_MAX_HZ     = 300.0f;  // up to open-B raised B (C4=261.6 Hz)
  static constexpr float HI_E_MIN_HZ  = 300.0f;  // down to open-E 1st string (E4=329.6 Hz)
  static constexpr float HI_E_MAX_HZ  = 400.0f;  // up to open-E raised E (F4=349.2 Hz)

  static constexpr int MEDIAN_FRAMES = 5;
  static float         freqHistoryLowE[MEDIAN_FRAMES];
  static int           histCountLowE;
  static bool          reportedLowE;
  static float         freqHistoryA[MEDIAN_FRAMES];
  static int           histCountA;
  static bool          reportedA;
  static float         freqHistoryD[MEDIAN_FRAMES];
  static int           histCountD;
  static bool          reportedD;
  static float         freqHistoryG[MEDIAN_FRAMES];
  static int           histCountG;
  static bool          reportedG;
  static float         freqHistoryB[MEDIAN_FRAMES];
  static int           histCountB;
  static bool          reportedB;
  static float         freqHistoryHiE[MEDIAN_FRAMES];
  static int           histCountHiE;
  static bool          reportedHiE;

 public:
  static String getNoteName(float freq);

  // Calculate cents off
  static int getCentsOff(float freq);

  // Insertion-sort median over n values
  static float medianFreq(float* arr, int n);

  // Process one strings ready buffer, i.e. silence-check, pitch detect, median filter, print
  static void processString(Tuner& t, float minHz, float maxHz, float* history, int& histCount,
      bool& reported, const char* label);

  static void printFun(const char* label, float freq);

  // ISR: one conversion captures all channels simultaneously on the ADC
  static void sampleISR();

  static bool noneReady();

  static void setup();

  static void loop();

 private:
  // Array of note names
  static constexpr const char* noteNames[12] = {
      "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
};

#endif  // PROCESSING_H