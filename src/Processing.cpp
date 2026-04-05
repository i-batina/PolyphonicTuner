#include "Processing.h"
#include "ADCDriver.h"
#include "MotorControl.h"
#include "oled/OledScript.h"
#include "oled/ui.h"

#include <IntervalTimer.h>
#include <math.h>

Tuner         tunerLowE("Low E");
Tuner         tunerA("A");
Tuner         tunerD("D");
Tuner         tunerG("G");
Tuner         tunerB("B");
Tuner         tunerHiE("High E");
ADCDriver     adc;
static Tuner* _tunerPtrs[6];
IntervalTimer sampleTimer;
MotorControl  motor;
OledScript    oledScript;

String Processing::getNoteName(float freq) {
  // Handle silence/noise
  if (freq < 20) return "---";

  // Calculate MIDI note number based on A4 = 69
  float noteNumFloat = 69 + 12 * log2(freq / 440.0);
  int   noteNum      = (int)(noteNumFloat + 0.5);

  // Get note index (0-11)
  int noteIndex = noteNum % 12;

  // Calculate octave
  int octave = (noteNum / 12) - 1;

  // Format result
  return String(_noteNames[noteIndex]) + String(octave);
}

int Processing::getCentsOff(float freq) {
  float noteNumFloat = 69 + 12 * log2(freq / 440.0);
  int   noteNum      = (int)(noteNumFloat + 0.5);
  float diff         = noteNumFloat - noteNum;
  return (int)(diff * 100);
}

float Processing::getCentsOffTarget(float targetFreq, float freq) {
  float targetNoteNumFloat = 69 + 12 * log2(targetFreq / 440.0);
  float noteNumFloat       = 69 + 12 * log2(freq / 440.0);
  float diff               = noteNumFloat - targetNoteNumFloat;
  return diff * 100;
}

float Processing::medianFreq(float* arr, int n) {
  float sorted[MEDIAN_FRAMES] = {};
  for (int i = 0; i < n; i++) sorted[i] = arr[i];
  for (int i = 1; i < n; i++) {
    float key = sorted[i];
    int   j   = i - 1;
    while (j >= 0 && sorted[j] > key) {
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = key;
  }
  return sorted[n / 2];
}

float Processing::processString(Tuner& t, float minHz, float maxHz, float* history, int& histCount,
    bool& reported, const char* label, float targetFreq) {
  int16_t p2p = t.peakToPeak();

  if (p2p <= 200) {
    // True silence: reset everything
    histCount           = 0;
    reported            = false;
    t.minP2PSinceReport = INT16_MAX;
    return 0.0f;
  }

  // String is active (p2p > 200)
  if (reported) {
    // Onset detection: track the post-report amplitude minimum, then watch for
    // a >=50% upward jump signalling a fresh pluck. This unblocks re-detection
    // without waiting for slow-decaying strings (B, Hi E) to fully go silent.
    if (p2p < t.minP2PSinceReport) t.minP2PSinceReport = p2p;
    if (p2p > t.minP2PSinceReport * 1.5f) {
      // New pluck — reset and wait for the next clean buffer
      histCount           = 0;
      reported            = false;
      t.minP2PSinceReport = INT16_MAX;
    }
    return 0.0f;
  }

  // Not yet reported: detect pitch
  t.removeDC();
  float freq = t.detectPitch(SAMPLE_RATE);
  if (freq > minHz && freq < maxHz) {
    history[histCount++] = freq;
    if (histCount >= MEDIAN_FRAMES) {
      float stableFreq    = medianFreq(history, MEDIAN_FRAMES);
      histCount           = 0;
      reported            = true;
      t.minP2PSinceReport = INT16_MAX;  // begin tracking post-report minimum
      // printFun(label, stableFreq, targetFreq);
      // tunePrintHelper(fabs(stableFreq - targetFreq) < 1.0f, stableFreq, targetFreq);
      return stableFreq;
    }
  }
  return 0.0f;
}

void Processing::sampleISR() {
  adc.startConversion();
  delayMicroseconds(5);  // tCONV max = 4.2 us (AD7606, 8-ch, no oversampling)

  int16_t samples[6];
  adc.readChannels(samples, 6);
  tunerLowE.addSample(samples[0]);  // ADC 1 Low E
  tunerA.addSample(samples[0]);     // ADC 2 A
  tunerD.addSample(samples[0]);     // ADC 3 D
  tunerG.addSample(samples[2]);     // ADC 4 G
  tunerB.addSample(samples[2]);     // ADC 5 B
  tunerHiE.addSample(samples[2]);   // ADC 6 Hi E
}

void Processing::setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  // Flag if nothing prints
  if (CrashReport) {
    Serial.print(CrashReport);  // Teensy 4.x feature, might be useful
  }

  Serial.println("STARTING PITCH DETECTOR...");

  motor.setup();
  adc.begin();
  _tunerPtrs[0] = &tunerLowE;
  _tunerPtrs[1] = &tunerA;
  _tunerPtrs[2] = &tunerD;
  _tunerPtrs[3] = &tunerG;
  _tunerPtrs[4] = &tunerB;
  _tunerPtrs[5] = &tunerHiE;

  // 16kHz => 62.5 us period
  sampleTimer.begin(sampleISR, 62.5);

  oledScript.setup();

  Serial.println("SAMPLING STARTED");
}

void Processing::loop() {
  oledScript.loop();

  bool active = UI::isTuningActive();

  if (!active) {
    if (_tuningWasActive) {
      // user backed out, reset all per-string state
      for (int i = 0; i < 6; i++) {
        _histCount[i] = 0;
        _reported[i]  = false;
        _tunerPtrs[i]->reset();
        memset(_freqHistory[i], 0, sizeof(_freqHistory[i]));
      }
      _currentStringIdx = 0;
      _tuningWasActive  = false;
      motor.stopAllMotors();
    }
    return;
  }

  if (!_tuningWasActive) {
    // first frame after "Start" pressed, show pluck prompt for string 0
    _tuningWasActive = true;
    int tuningIdx    = UI::getSelectedTuningIndex();
    UI::showPluckPrompt(
        STR_ORDINALS[0], STR_LABELS[0], _tunings[tuningIdx].notes[0], _tunings[tuningIdx].freqs[0]);
    return;
  }

  if (_currentStringIdx >= 6) return;  // all done, ui shows SCREEN_ALL_TUNED

  int    i         = _currentStringIdx;
  int    tuningIdx = UI::getSelectedTuningIndex();
  float  targetHz  = _tunings[tuningIdx].freqs[i];
  Tuner* t         = _tunerPtrs[i];

  float freq = processString(*t, getMinHz(i), getMaxHz(i), _freqHistory[i], _histCount[i],
      _reported[i], STR_LABELS[i], targetHz);

  if (freq > 0.0f) {
    String curNote = getNoteName(freq);
    String tgtNote = getNoteName(targetHz);
    UI::setTuningDisplay(curNote.c_str(), tgtNote.c_str(), STR_LABELS[i], freq > targetHz);
    UI::showTuningScreen();

    float cents    = getCentsOffTarget(targetHz, freq);
    float absCents = fabsf(cents);

    if (absCents < 10.0f) {
      // In tune, brief display of tuning screen, then advance
      delay(600);
      _currentStringIdx++;
      if (_currentStringIdx >= 6) {
        UI::signalAllTuned();
      } else {
        int ni = _currentStringIdx;
        UI::showPluckPrompt(STR_ORDINALS[ni], STR_LABELS[ni], _tunings[tuningIdx].notes[ni],
            _tunings[tuningIdx].freqs[ni]);
      }
    } else {
      // TODO: consider:
      // 1. Add "Tuning up/down" text to the UI
      // 2. Add real time frequency and cents display to the UI? maybe for debugging
      // 3. let user skip tuning a string by pressing next and back without tuning to target, in
      // case of broken string or other issue
      float norm   = constrain((absCents - 2.0f) / (CENTS_MAX_CLAMP - 2.0f), 0.0f, 1.0f);
      int   spinMs = MIN_SPINS_MS + (int)(norm * (MAX_SPINS_MS - MIN_SPINS_MS));
      motor.tune(targetHz, freq, i);
      delay(spinMs);  // brief delay to allow motor response before processing next string
      motor.stopAllMotors();
      delay(600);  // wait for string to stop vibrating from motor before next pitch read

      _reported[i]  = false;  // allow fresh pitch reading w/o re plucking
      _histCount[i] = 0;      // reset median history for fresh reading
      // Go back to pluck prompt state for same string
      UI::showPluckPrompt(STR_ORDINALS[i], STR_LABELS[i], _tunings[tuningIdx].notes[i],
          _tunings[tuningIdx].freqs[i]);
    }
  }

  t->reset();  // TODO: move into if req > 0?
  sampleTimer.begin(sampleISR, 62.5);
}

// Helpers
void Processing::printFun(const char* label, float freq, float targetFreq) {
  Serial.print("[");
  Serial.print(label);
  Serial.print("] Freq: ");
  Serial.print(freq, 2);
  Serial.print(" Hz  |  Note: ");
  Serial.print(getNoteName(freq));
  Serial.print("  |  Cents: ");
  Serial.println(getCentsOffTarget(targetFreq, freq));
}

void Processing::tunePrintHelper(bool isInTune, float freq, float targetFreq) {
  if (isInTune) {
    Serial.println("In Tune!");
    return;
  } else if ((freq - targetFreq) < 0.0f) {
    Serial.println("Tune Up!");
    return;
  } else {
    Serial.println("Tune Down!");
    return;
  }
}

float Processing::getMinHz(int strIdx) { return MIN_HZ[strIdx]; }
float Processing::getMaxHz(int strIdx) { return MAX_HZ[strIdx]; }
int   Processing::getSizeTuneLst() { return sizeof(_tunings) / sizeof(Tuning); }
