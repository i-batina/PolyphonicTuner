#include <Arduino.h>
#include <IntervalTimer.h>
#include <math.h>

#include "ADCDriver.h"
#include "Tuner.h"

ADCDriver adc;
Tuner tunerLowE("Low E");
Tuner tunerA("A");

IntervalTimer sampleTimer;

static constexpr float SAMPLE_RATE = 16000.0f;

// Frequency gates per string wide enough to cover standard and common alternate tunings
static constexpr float LOW_E_MIN_HZ = 50.0f;  // Covers down to Drop A (55 Hz)
static constexpr float LOW_E_MAX_HZ = 90.0f;  // Covers up to open E (82.41 Hz) with headroom
static constexpr float A_MIN_HZ = 85.0f;      // Covers down to open-G 5th string (G2=98 Hz)
static constexpr float A_MAX_HZ = 135.0f;     // Covers up to open-E raised A (B2=123.5 Hz)

// Array of note names
const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

String getNoteName(float freq) {
  // Handle silence/noise
  if (freq < 20) return "---";

  // Calculate MIDI note number based on A4 = 69
  float noteNumFloat = 69 + 12 * log2(freq / 440.0);
  int noteNum = (int)(noteNumFloat + 0.5);

  // Get note index (0-11)
  int noteIndex = noteNum % 12;

  // Calculate octave
  int octave = (noteNum / 12) - 1;

  // Format result
  return String(noteNames[noteIndex]) + String(octave);
}

// Calculate cents off
int getCentsOff(float freq) {
  float noteNumFloat = 69 + 12 * log2(freq / 440.0);
  int noteNum = (int)(noteNumFloat + 0.5);
  float diff = noteNumFloat - noteNum;
  return (int)(diff * 100);
}

static constexpr int MEDIAN_FRAMES = 5;
static float freqHistoryLowE[MEDIAN_FRAMES];
static int histCountLowE = 0;
static float freqHistoryA[MEDIAN_FRAMES];
static int histCountA = 0;

// Insertion-sort median over n values
float medianFreq(float* arr, int n) {
  float sorted[MEDIAN_FRAMES];
  for (int i = 0; i < n; i++) sorted[i] = arr[i];
  for (int i = 1; i < n; i++) {
    float key = sorted[i];
    int j = i - 1;
    while (j >= 0 && sorted[j] > key) {
      sorted[j + 1] = sorted[j];
      j--;
    }
    sorted[j + 1] = key;
  }
  return sorted[n / 2];
}

// Process one strings ready buffer, i.e. silence-check, pitch detect, median filter, print
void processString(
    Tuner& t, float minHz, float maxHz, float* history, int& histCount, const char* label) {
  if (t.peakToPeak() > 500) {
    t.removeDC();
    float freq = t.detectPitch(SAMPLE_RATE);
    if (freq > minHz && freq < maxHz) {
      history[histCount++] = freq;
      if (histCount >= MEDIAN_FRAMES) {
        float stableFreq = medianFreq(history, MEDIAN_FRAMES);
        histCount = 0;
        Serial.print("[");
        Serial.print(label);
        Serial.print("] Freq: ");
        Serial.print(stableFreq, 2);
        Serial.print(" Hz  |  Note: ");
        Serial.print(getNoteName(stableFreq));
        Serial.print("  |  Cents: ");
        Serial.println(getCentsOff(stableFreq));
      }
    }
  } else {
    histCount = 0;  // String went silent -> discard partial history
  }
}

// ISR: one conversion captures all channels simultaneously on the ADC
void sampleISR() {
  adc.startConversion();
  delayMicroseconds(5);  // tCONV max = 4.2 us (AD7606, 8-ch, no oversampling)

  int16_t samples[2];
  adc.readChannels(samples, 2);  // samples[0]=CH1 (Low E), samples[1]=CH2 (A)
  tunerLowE.addSample(samples[0]);
  tunerA.addSample(samples[1]);
}

void setup() {
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

void loop() {
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
