#include "ADCDriver.h"
#include "Tuner.h"
#include <Arduino.h>
#include <IntervalTimer.h>
#include <math.h>

ADCDriver adc;
Tuner tuner("Monophonic");

IntervalTimer sampleTimer;

static constexpr float SAMPLE_RATE = 16000.0f;

static constexpr float LOW_E_MIN_HZ = 50.0f;
static constexpr float LOW_E_MAX_HZ = 90.0f;

// Array of note names
const char *noteNames[] = {"C",  "C#", "D",  "D#", "E",  "F",
                           "F#", "G",  "G#", "A",  "A#", "B"};

String getNoteName(float freq) {
  // Handle silence/noise
  if (freq < 20)
    return "---";

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

// ISR
void sampleISR() {
  adc.startConversion();

  if (!adc.waitBusy())
    return; // NOTE: level shifter bricks BUSY pin

  int16_t s = adc.readChannel(2); // Read low E string (channel 2 for now)
  tuner.addSample(s);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  // Flag if nothing prints
  if (CrashReport) {
    Serial.print(CrashReport); // Teensy 4.x feature, might be useful
  }

  Serial.println("STARTING PITCH DETECTOR...");

  adc.begin();

  // 16kHz => 62.5 us period
  sampleTimer.begin(sampleISR, 62.5);

  Serial.println("SAMPLING STARTED");
}

void loop() {
  if (!tuner.isReady())
    return;
  sampleTimer.end();

  int16_t p2p = tuner.peakToPeak();

  // Threshold to kill silence noise
  if (p2p > 500) {
    tuner.removeDC();
    float freq = tuner.detectPitch(SAMPLE_RATE);

    // Min and max Freq to kill ghost notes and octave errors
    if (freq > LOW_E_MIN_HZ && freq < LOW_E_MAX_HZ) {
      String note = getNoteName(freq);
      int cents = getCentsOff(freq);

      Serial.print("Freq: ");
      Serial.print(freq);
      Serial.print(" Hz  |  Note: ");
      Serial.print(note);
      Serial.print("  |  Cents: ");
      Serial.println(cents);
    }
  }

  tuner.reset();
  sampleTimer.begin(sampleISR, 62.5);
}
