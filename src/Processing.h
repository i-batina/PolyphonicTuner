#ifndef PROCESSING_H
#define PROCESSING_H

#include <Arduino.h>
#include "Tuner.h"

struct Tuning {
  const char* name;
  const char* notes[6];
  float       freqs[6];
};

class Processing {
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
  // static void  loopProcessHelper(Tuner& tuner, int strIdx, float* freqHistory, int& histCount,
  //     bool& reported, const char* label, float targetFreq);
  // static bool  noneReady();
  static void  tunePrintHelper(bool isInTune, float targetFreq, float freq);
  static float getMinHz(int strIdx);
  static float getMaxHz(int strIdx);

 private:
  static constexpr float SAMPLE_RATE     = 16000.0f;
  static constexpr int   MEDIAN_FRAMES   = 5;
  static constexpr int   MIN_SPINS_MS    = 60;     // TODO: tune this param based on testing
  static constexpr int   MAX_SPINS_MS    = 600;    // keep for large deviations
  static constexpr float CENTS_MAX_CLAMP = 50.0f;  // TODO: tune this param based on testing

  // Frequency gates — wide enough to cover all four alternate tunings
  static constexpr float MIN_HZ[6] = {50.0f, 95.0f, 125.0f, 165.0f, 215.0f, 280.0f};
  static constexpr float MAX_HZ[6] = {90.0f, 120.0f, 160.0f, 210.0f, 260.0f, 360.0f};

  // String labels and ordinal names (index 0 = Low E = 6th string)
  static constexpr const char* STR_LABELS[6]   = {"Low E", "A", "D", "G", "B", "High E"};
  static constexpr const char* STR_ORDINALS[6] = {"6th", "5th", "4th", "3rd", "2nd", "1st"};

  // Per-string pitch-detection state (arrays replace the old per-name fields)
  inline static float _freqHistory[6][MEDIAN_FRAMES] = {};
  inline static int   _histCount[6]                  = {};
  inline static bool  _reported[6]                   = {};

  // Sequential tuning state
  inline static int  _currentStringIdx = 0;
  inline static bool _tuningWasActive  = false;

  // Alternate tunings (Standard, Drop D, Open G, Open D)
  static constexpr Tuning _tunings[] = {
      {"Standard", {"E", "A", "D", "G", "B", "e"},
          {82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f}},
      {"Drop D", {"D", "A", "D", "G", "B", "e"},
          {73.42f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f}},
      {"Open G", {"D", "G", "D", "G", "B", "D"},
          {73.42f, 98.0f, 146.83f, 196.0f, 246.94f, 293.66f}},
      {"Open D", {"D", "A", "D", "F#", "A", "D"},
          {73.42f, 110.0f, 146.83f, 185.0f, 220.0f, 293.66f}},
  };

  static constexpr const char* _noteNames[12] = {
      "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
};

#endif  // PROCESSING_H