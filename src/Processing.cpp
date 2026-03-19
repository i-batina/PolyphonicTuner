#include "Processing.h"
#include "ADCDriver.h"

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
  return String(noteNames[noteIndex]) + String(octave);
}

int Processing::getCentsOff(float freq) {
  float noteNumFloat = 69 + 12 * log2(freq / 440.0);
  int   noteNum      = (int)(noteNumFloat + 0.5);
  float diff         = noteNumFloat - noteNum;
  return (int)(diff * 100);
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

void Processing::processString(Tuner& t, float minHz, float maxHz, float* history, int& histCount,
    bool& reported, const char* label) {
  int16_t p2p = t.peakToPeak();

  if (p2p <= 200) {
    // True silence: reset everything
    histCount           = 0;
    reported            = false;
    t.minP2PSinceReport = INT16_MAX;
    return;
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
    return;
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
      printFun(label, stableFreq);
    }
  }
}

// print helper
void Processing::printFun(const char* label, float freq) {
  Serial.print("[");
  Serial.print(label);
  Serial.print("] Freq: ");
  Serial.print(freq, 2);
  Serial.print(" Hz  |  Note: ");
  Serial.print(getNoteName(freq));
  Serial.print("  |  Cents: ");
  Serial.println(getCentsOff(freq));
}

void Processing::sampleISR() {
  adc.startConversion();
  delayMicroseconds(5);  // tCONV max = 4.2 us (AD7606, 8-ch, no oversampling)

  int16_t samples[2];
  adc.readChannels(samples, 2);  // samples[0]=CH1 (Low E), samples[1]=CH2 (A)
  tunerLowE.addSample(samples[1]);
  // tunerA.addSample(samples[1]);
  // tunerD.addSample(samples[1]);
  // tunerG.addSample(samples[1]);
  // tunerB.addSample(samples[1]);
  // tunerHiE.addSample(samples[1]);
}

bool Processing::noneReady() {
  return !tunerLowE.isReady() && !tunerA.isReady() && !tunerD.isReady() && !tunerG.isReady() &&
         !tunerB.isReady() && !tunerHiE.isReady();
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

  adc.begin();

  // 16kHz => 62.5 us period
  sampleTimer.begin(sampleISR, 62.5);

  Serial.println("SAMPLING STARTED");
}

void Processing::loop() {
  if (noneReady()) return;
  sampleTimer.end();

  if (tunerLowE.isReady()) {
    processString(
        tunerLowE, getMinHz(0), getMaxHz(0), freqHistoryLowE, histCountLowE, reportedLowE, "Low E");
    tunerLowE.reset();
  }

  if (tunerA.isReady()) {
    processString(tunerA, getMinHz(1), getMaxHz(1), freqHistoryA, histCountA, reportedA, "A");
    tunerA.reset();
  }

  if (tunerD.isReady()) {
    processString(tunerD, getMinHz(2), getMaxHz(2), freqHistoryD, histCountD, reportedD, "D");
    tunerD.reset();
  }

  if (tunerG.isReady()) {
    processString(tunerG, getMinHz(3), getMaxHz(3), freqHistoryG, histCountG, reportedG, "G");
    tunerG.reset();
  }

  if (tunerB.isReady()) {
    processString(tunerB, getMinHz(4), getMaxHz(4), freqHistoryB, histCountB, reportedB, "B");
    tunerB.reset();
  }

  if (tunerHiE.isReady()) {
    processString(
        tunerHiE, getMinHz(5), getMaxHz(5), freqHistoryHiE, histCountHiE, reportedHiE, "High E");
    tunerHiE.reset();
  }

  sampleTimer.begin(sampleISR, 62.5);
}

// Helpers

float Processing::getMinHz(int strIdx) { return MIN_HZ[strIdx]; }

float Processing::getMaxHz(int strIdx) { return MAX_HZ[strIdx]; }