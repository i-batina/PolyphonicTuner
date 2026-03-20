#ifndef PROCESSING_H
#define PROCESSING_H

#include <Arduino.h>
#include "Tuner.h"

class Processing {
 private:
  static constexpr float SAMPLE_RATE = 16000.0f;

  // E, A, D, G, B, e frequency ranges (Hz) for pitch detection gate
  static constexpr float MIN_HZ[6] = {50.0f, 95.0f, 125.0f, 165.0f, 215.0f, 280.0f};
  static constexpr float MAX_HZ[6] = {90.0f, 120.0f, 160.0f, 210.0f, 260.0f, 360.0f};

  static constexpr int   MIN_SPINS_MS    = 60;     // spin time when just outside deadband, in ms
  static constexpr int   MAX_SPINS_MS    = 600;    // spin time when well outside deadband, in ms
  static constexpr float CENTS_MAX_CLAMP = 50.0f;  // cents at which spin is capped at MAX_SPINS_MS

  // Frequency gates per string wide enough to cover standard and common alternate tunings
  // open E (82.41 Hz)
  // open A 5th string (A2=110 Hz)
  // open-D 4th string (D3=146.8 Hz)
  // open-G 3rd string (G3=196.0 Hz)
  // open-B 2nd string (B3=246.9 Hz)
  // open E 1st string (E4=329.6 Hz)
  static constexpr int MEDIAN_FRAMES = 5;

  inline static float _freqHistoryLowE[MEDIAN_FRAMES] = {};
  inline static float _freqHistoryA[MEDIAN_FRAMES]    = {};
  inline static float _freqHistoryD[MEDIAN_FRAMES]    = {};
  inline static float _freqHistoryG[MEDIAN_FRAMES]    = {};
  inline static float _freqHistoryB[MEDIAN_FRAMES]    = {};
  inline static float _freqHistoryHiE[MEDIAN_FRAMES]  = {};

  inline static int _histCountLowE = 0;
  inline static int _histCountA    = 0;
  inline static int _histCountD    = 0;
  inline static int _histCountG    = 0;
  inline static int _histCountB    = 0;
  inline static int _histCountHiE  = 0;

  inline static bool _reportedLowE = false;
  inline static bool _reportedA    = false;
  inline static bool _reportedD    = false;
  inline static bool _reportedG    = false;
  inline static bool _reportedB    = false;
  inline static bool _reportedHiE  = false;

 public:
  static String getNoteName(float freq);

  // Calculate cents off
  static int   getCentsOff(float freq);
  static float getCentsOffTarget(float targetFreq, float freq);

  // Insertion-sort median over n values
  static float medianFreq(float* arr, int n);

  // Process one string's ready buffer. Returns stable median frequency when a new
  // reading is ready, 0.0 otherwise.
  static float processString(Tuner& t, float minHz, float maxHz, float* history, int& histCount,
      bool& reported, const char* label, float targetFreq);

  static void printFun(const char* label, float freq, float targetFreq);

  // ISR: one conversion captures all channels simultaneously on the ADC
  static void sampleISR();

  static void setup();

  static void loop();

  // Helpers
  static void  loopProcessHelper(Tuner& tuner, int strIdx, float* freqHistory, int& histCount,
       bool& reported, const char* label, float targetFreq);
  static bool  noneReady();
  static void  tunePrintHelper(bool isInTune, float targetFreq, float freq);
  static float getMinHz(int strIdx);
  static float getMaxHz(int strIdx);

 private:
  // Array of note names
  static constexpr const char* _noteNames[12] = {
      "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
};

#endif  // PROCESSING_H