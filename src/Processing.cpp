#include "Processing.h"
#include "ADCDriver.h"

#include <IntervalTimer.h>
#include <math.h>

// Out-of-class definitions for non-constexpr static members
float Processing::freqHistoryLowE[Processing::MEDIAN_FRAMES];
int   Processing::histCountLowE = 0;
float Processing::freqHistoryA[Processing::MEDIAN_FRAMES];
int   Processing::histCountA = 0;

Tuner         tunerLowE("Low E");
Tuner         tunerA("A");
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
  float sorted[MEDIAN_FRAMES];
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

void Processing::processString(
    Tuner& t, float minHz, float maxHz, float* history, int& histCount, const char* label) {
  if (t.peakToPeak() > 500) {
    t.removeDC();
    float freq = t.detectPitch(SAMPLE_RATE);
    if (freq > minHz && freq < maxHz) {
      history[histCount++] = freq;
      if (histCount >= MEDIAN_FRAMES) {
        float stableFreq = medianFreq(history, MEDIAN_FRAMES);
        histCount        = 0;
        printFun(label, stableFreq);
      }
    }
  } else {
    histCount = 0;  // String went silent -> discard partial history
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
  tunerLowE.addSample(samples[0]);
  tunerA.addSample(samples[1]);
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
  if (!tunerLowE.isReady() && !tunerA.isReady()) return;
  sampleTimer.end();

  if (tunerLowE.isReady()) {
    processString(tunerLowE, LOW_E_MIN_HZ, LOW_E_MAX_HZ, freqHistoryLowE, histCountLowE, "Low E");
    tunerLowE.reset();
  }

  if (tunerA.isReady()) {
    processString(tunerA, A_MIN_HZ, A_MAX_HZ, freqHistoryA, histCountA, "A");
    tunerA.reset();
  }

  sampleTimer.begin(sampleISR, 62.5);
}