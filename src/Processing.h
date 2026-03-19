#ifndef PROCESSING_H
#define PROCESSING_H

#include <Arduino.h>
#include "Tuner.h"

class Processing {
 private:
  static constexpr float SAMPLE_RATE = 16000.0f;

  // E, A, D, G, B, e frequency ranges (Hz) for pitch detection gate
  static constexpr float MIN_HZ[6] = {50.0f, 95.0f, 120.0f, 150.0f, 200.0f, 300.0f};
  static constexpr float MAX_HZ[6] = {90.0f, 135.0f, 200.0f, 250.0f, 300.0f, 400.0f};

  // Frequency gates per string wide enough to cover standard and common alternate tunings
  // open E (82.41 Hz)
  // open A 5th string (A2=110 Hz)
  // open-D 4th string (D3=146.8 Hz)
  // open-G 3rd string (G3=196.0 Hz)
  // open-B 2nd string (B3=246.9 Hz)
  // open E 1st string (E4=329.6 Hz)

  static constexpr int MEDIAN_FRAMES                  = 5;
  inline static float  freqHistoryLowE[MEDIAN_FRAMES] = {};
  inline static int    histCountLowE                  = 0;
  inline static bool   reportedLowE                   = false;
  inline static float  freqHistoryA[MEDIAN_FRAMES]    = {};
  inline static int    histCountA                     = 0;
  inline static bool   reportedA                      = false;
  inline static float  freqHistoryD[MEDIAN_FRAMES]    = {};
  inline static int    histCountD                     = 0;
  inline static bool   reportedD                      = false;
  inline static float  freqHistoryG[MEDIAN_FRAMES]    = {};
  inline static int    histCountG                     = 0;
  inline static bool   reportedG                      = false;
  inline static float  freqHistoryB[MEDIAN_FRAMES]    = {};
  inline static int    histCountB                     = 0;
  inline static bool   reportedB                      = false;
  inline static float  freqHistoryHiE[MEDIAN_FRAMES]  = {};
  inline static int    histCountHiE                   = 0;
  inline static bool   reportedHiE                    = false;

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

  // Helpers
  static float getMinHz(int strIdx);
  static float getMaxHz(int strIdx);

 private:
  // Array of note names
  static constexpr const char* noteNames[12] = {
      "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
};

#endif  // PROCESSING_H