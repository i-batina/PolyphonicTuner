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
      printFun(label, stableFreq, targetFreq);
      tunePrintHelper(fabs(stableFreq - targetFreq) < 1.0f, stableFreq, targetFreq);
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
  tunerLowE.addSample(samples[0]);
  tunerA.addSample(samples[2]);
  tunerD.addSample(samples[1]);
  tunerG.addSample(samples[3]);
  tunerB.addSample(samples[4]);
  tunerHiE.addSample(samples[5]);
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

  // 16kHz => 62.5 us period
  sampleTimer.begin(sampleISR, 62.5);

  oledScript.setup();

  Serial.println("SAMPLING STARTED");
}

void Processing::loop() {
  oledScript.loop();

  if (noneReady()) return;
  sampleTimer.end();

  if (tunerLowE.isReady())
    loopProcessHelper(
        tunerLowE, 0, _freqHistoryLowE, _histCountLowE, _reportedLowE, "Low E", 82.41f);

  if (tunerA.isReady())
    loopProcessHelper(tunerA, 1, _freqHistoryA, _histCountA, _reportedA, "A", 110.0f);

  if (tunerD.isReady())
    loopProcessHelper(tunerD, 2, _freqHistoryD, _histCountD, _reportedD, "D", 146.8f);

  if (tunerG.isReady())
    loopProcessHelper(tunerG, 3, _freqHistoryG, _histCountG, _reportedG, "G", 196.0f);

  if (tunerB.isReady()) {
    loopProcessHelper(tunerB, 4, _freqHistoryB, _histCountB, _reportedB, "B", 246.9f);
  }

  if (tunerHiE.isReady())
    loopProcessHelper(tunerHiE, 5, _freqHistoryHiE, _histCountHiE, _reportedHiE, "High E", 329.6f);

  sampleTimer.begin(sampleISR, 62.5);
}

// Helpers
void Processing::loopProcessHelper(Tuner& tuner, int strIdx, float* freqHistory, int& histCount,
    bool& reported, const char* label, float targetFreq) {
  float freq = processString(tuner, getMinHz(strIdx), getMaxHz(strIdx), freqHistory, histCount,
      reported, label, targetFreq);
  if (freq > 0.0f) {
    // Update OLED tuning screen with live pitch data
    String currentNoteStr = getNoteName(freq);
    String targetNoteStr  = getNoteName(targetFreq);
    UI::setTuningDisplay(currentNoteStr.c_str(), targetNoteStr.c_str(), label, freq > targetFreq);

    float absCents = fabsf(getCentsOffTarget(targetFreq, freq));
    float t        = constrain((absCents - 2.0f) / (CENTS_MAX_CLAMP - 2.0f), 0.0f, 1.0f);
    int   spinMs   = MIN_SPINS_MS + (int)(t * (MAX_SPINS_MS - MIN_SPINS_MS));

    motor.tune(targetFreq, freq, strIdx);
    delay(spinMs);  // brief delay to allow motor response before processing next string
    motor.stopAllMotors();
    delay(600);         // wait for string to stop vibrating from motor
    reported  = false;  // allow fresh pitch reading w/o re plucking
    histCount = 0;      // reset median history for fresh reading
  }
  tuner.reset();
}

bool Processing::noneReady() {
  return !tunerLowE.isReady() && !tunerA.isReady() && !tunerD.isReady() && !tunerG.isReady() &&
         !tunerB.isReady() && !tunerHiE.isReady();
}
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