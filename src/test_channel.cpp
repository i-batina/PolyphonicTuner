// Channel diagnostic test — compiled only when TEST_CHANNEL is defined.
//
// Upload with:   pio run -e teensy41_ch1 -t upload   (or ch2... ch6)
// Monitor with:  pio device monitor
//
// Build envs defined in platformio.ini:
// TODO: make sure channels and strings match:
//  teensy41_ch1:      CH1, Low E  (target 82.41 Hz)
//  teensy41_ch2:      CH2, A      (target 110.00 Hz)
//  teensy41_ch3:       CH3, D      (target 146.83 Hz)
//  teensy41_ch4:       CH4, G      (target 196.00 Hz)
//  teensy41_ch5:       CH5, B      (target 246.94 Hz)
//  teensy41_ch6:       CH6, High E (target 329.63 Hz)
//  teensy41_accuracy: Low E accuracy benchmark (100 plucks → stats)

#ifdef TEST_CHANNEL

#include <Arduino.h>
#include <IntervalTimer.h>
#include "ADCDriver.h"
#include "Processing.h"
#include "Tuner.h"

// -- Per-channel configuration ------------------------------------------------
#if TEST_CHANNEL == 1
static constexpr const char* LABEL     = "Low E";
static constexpr float       TARGET_HZ = 82.41f;
static constexpr float       GATE_MIN  = 50.0f;
static constexpr float       GATE_MAX  = 90.0f;
static constexpr uint8_t     ADC_CH    = 1;

#elif TEST_CHANNEL == 2
static constexpr const char* LABEL     = "A";
static constexpr float       TARGET_HZ = 110.0f;
static constexpr float       GATE_MIN  = 95.0f;
static constexpr float       GATE_MAX  = 135.0f;
static constexpr uint8_t     ADC_CH    = 2;

#elif TEST_CHANNEL == 3
static constexpr const char* LABEL     = "D";
static constexpr float       TARGET_HZ = 146.83f;
static constexpr float       GATE_MIN  = 120.0f;
static constexpr float       GATE_MAX  = 200.0f;
static constexpr uint8_t     ADC_CH    = 3;

#elif TEST_CHANNEL == 4
static constexpr const char* LABEL     = "G";
static constexpr float       TARGET_HZ = 196.0f;
static constexpr float       GATE_MIN  = 150.0f;
static constexpr float       GATE_MAX  = 250.0f;
static constexpr uint8_t     ADC_CH    = 4;

#elif TEST_CHANNEL == 5
static constexpr const char* LABEL     = "B";
static constexpr float       TARGET_HZ = 246.94f;
static constexpr float       GATE_MIN  = 200.0f;
static constexpr float       GATE_MAX  = 300.0f;
static constexpr uint8_t     ADC_CH    = 5;

#elif TEST_CHANNEL == 6
static constexpr const char* LABEL     = "High E";
static constexpr float       TARGET_HZ = 329.63f;
static constexpr float       GATE_MIN  = 300.0f;
static constexpr float       GATE_MAX  = 400.0f;
static constexpr uint8_t     ADC_CH    = 6;

#else
#error "TEST_CHANNEL must be 1 thru 6 (1=Low E, 2=A, 3=D, 4=G, 5=B, 6=High E)"
#endif

// -- Hardware objects ----------------------------------------------------------
static Tuner         tuner(LABEL);
static ADCDriver     adc;
static IntervalTimer sampleTimer;

// -- ISR: 16 kHz sample tick ---------------------------------------------------
// For CH2, both channels must be clocked out; we discard CH1 and keep CH2.
static void sampleISR() {
  adc.startConversion();
  delayMicroseconds(5);  // tCONV max 4.2 µs (AD7606)
  int16_t buf[ADC_CH];
  adc.readChannels(buf, ADC_CH);
  tuner.addSample(buf[ADC_CH - 1]);
}

// -- setup / loop ----------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  Serial.println("========================================");
  Serial.print("  Channel Test: ");
  Serial.println(LABEL);
  Serial.print("  Target : ");
  Serial.print(TARGET_HZ, 2);
  Serial.print(" Hz  |  Gate: ");
  Serial.print(GATE_MIN, 0);
  Serial.print(" – ");
  Serial.print(GATE_MAX, 0);
  Serial.println(" Hz");
  Serial.println("========================================");

  adc.begin();
  sampleTimer.begin(sampleISR, 62.5f);  // 1 / 62.5 µs = 16 kHz

  Serial.println("Sampling at 16 kHz. Play the string...\n");
}

void loop() {
  if (!tuner.isReady()) return;
  sampleTimer.end();

  int16_t p2p = tuner.peakToPeak();

  if (p2p <= 500) {
    Serial.println("SILENT  (p2p <= 500)  —  play the string");
  } else {
    tuner.removeDC();
    float freq = tuner.detectPitch(16000.0f);

    Serial.print("p2p=");
    Serial.print(p2p);
    Serial.print("  |  freq=");
    Serial.print(freq, 2);
    Serial.print(" Hz");
    Serial.print("  |  note=");
    Serial.print(Processing::getNoteName(freq));
    Serial.print("  |  cents=");
    Serial.print(Processing::getCentsOff(freq));

    if (freq >= GATE_MIN && freq <= GATE_MAX) {
      float err = freq - TARGET_HZ;
      Serial.print("  |  err=");
      if (err > 0.0f) Serial.print("+");
      Serial.print(err, 2);
      Serial.print(" Hz");
    } else {
      Serial.print("  |  [OUT OF GATE]");
    }

    Serial.println();
  }

  tuner.reset();
  sampleTimer.begin(sampleISR, 62.5f);
}

#endif  // TEST_CHANNEL

// -- Channel Scanner -----------------------------------------------------------
// Reads all 8 ADC channels and prints a p2p table every second.
// Channels with p2p > LIVE_THRESH are marked LIVE — plug sensors in one at a
// time to discover which physical channel they map to.
//
// Upload:  pio run -e teensy41_scan -t upload
// Monitor: pio device monitor

#ifdef SCAN_CHANNELS

#include <Arduino.h>
#include "ADCDriver.h"

static constexpr int     NUM_CH       = 8;
static constexpr int     SCAN_SAMPLES = 1024;  // ~25 ms of samples
static constexpr int16_t LIVE_THRESH  = 200;

static ADCDriver adc;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }
  Serial.println("=== ADC Channel Scanner ===");
  Serial.println("Pluck a string to see which channel lights up as LIVE.");
  Serial.println("Each block is one scan (~25 ms of samples).");
  adc.begin();
}

void loop() {
  int16_t minVal[NUM_CH], maxVal[NUM_CH];
  for (int ch = 0; ch < NUM_CH; ch++) {
    minVal[ch] = 32767;
    maxVal[ch] = -32768;
  }

  for (int s = 0; s < SCAN_SAMPLES; s++) {
    adc.startConversion();
    delayMicroseconds(5);  // tCONV max 4.2 µs
    int16_t buf[NUM_CH];
    adc.readChannels(buf, NUM_CH);
    for (int ch = 0; ch < NUM_CH; ch++) {
      if (buf[ch] < minVal[ch]) minVal[ch] = buf[ch];
      if (buf[ch] > maxVal[ch]) maxVal[ch] = buf[ch];
    }
  }

  for (int ch = 0; ch < NUM_CH; ch++) {
    int16_t p2p = maxVal[ch] - minVal[ch];
    Serial.print("CH");
    Serial.print(ch + 1);
    Serial.print("  p2p=");
    // Right-align 5 digits
    if (p2p < 10000) Serial.print(" ");
    if (p2p < 1000) Serial.print(" ");
    if (p2p < 100) Serial.print(" ");
    if (p2p < 10) Serial.print(" ");
    Serial.print(p2p);
    Serial.print("  ");
    Serial.println(p2p > LIVE_THRESH ? "<<< LIVE" : "silent");
  }
  Serial.println("---");

  delay(1000);  // pause between scans so output is readable
}

#endif  // SCAN_CHANNELS

// -- Accuracy Benchmark --------------------------------------------------------
// Collects 100 plucks of one string and prints:
//   - cents off for each pluck
//   - average cents off (signed; positive = sharp, negative = flat)
//   - % of readings within ±5 cents
//   - % of readings within ±10 cents
//
// Upload:  pio run -e teensy41_accuracy_lowe -t upload   (Low E)
//          pio run -e teensy41_accuracy_a    -t upload   (A)
//          pio run -e teensy41_accuracy_d    -t upload   (D)
//          pio run -e teensy41_accuracy_g    -t upload   (G)
//          pio run -e teensy41_accuracy_b    -t upload   (B)
//          pio run -e teensy41_accuracy_hie  -t upload   (High E)
// Monitor: pio device monitor

#ifdef ACCURACY_TEST

#include <Arduino.h>
#include <IntervalTimer.h>
#include <math.h>
#include "ADCDriver.h"
#include "Tuner.h"

#if ACCURACY_TEST == 1
static constexpr const char* LABEL     = "Low E";
static constexpr float       TARGET_HZ = 82.41f;
static constexpr float       GATE_MIN  = 50.0f;
static constexpr float       GATE_MAX  = 90.0f;
static constexpr uint8_t     ADC_CH    = 1;  // CH2 = buf[1], matches Processing.cpp

#elif ACCURACY_TEST == 2
static constexpr const char* LABEL     = "A";
static constexpr float       TARGET_HZ = 110.0f;
static constexpr float       GATE_MIN  = 95.0f;
static constexpr float       GATE_MAX  = 135.0f;
static constexpr uint8_t     ADC_CH    = 2;

#elif ACCURACY_TEST == 3
static constexpr const char* LABEL     = "D";
static constexpr float       TARGET_HZ = 146.83f;
static constexpr float       GATE_MIN  = 120.0f;
static constexpr float       GATE_MAX  = 200.0f;
static constexpr uint8_t     ADC_CH    = 3;

#elif ACCURACY_TEST == 4
static constexpr const char* LABEL     = "G";
static constexpr float       TARGET_HZ = 196.0f;
static constexpr float       GATE_MIN  = 150.0f;
static constexpr float       GATE_MAX  = 250.0f;
static constexpr uint8_t     ADC_CH    = 4;

#elif ACCURACY_TEST == 5
static constexpr const char* LABEL     = "B";
static constexpr float       TARGET_HZ = 246.94f;
static constexpr float       GATE_MIN  = 200.0f;
static constexpr float       GATE_MAX  = 300.0f;
static constexpr uint8_t     ADC_CH    = 5;

#elif ACCURACY_TEST == 6
static constexpr const char* LABEL     = "High E";
static constexpr float       TARGET_HZ = 329.63f;
static constexpr float       GATE_MIN  = 300.0f;
static constexpr float       GATE_MAX  = 400.0f;
static constexpr uint8_t     ADC_CH    = 6;

#else
#error "ACCURACY_TEST must be 1-6 (1=Low E, 2=A, 3=D, 4=G, 5=B, 6=High E)"
#endif

static constexpr int TOTAL_PLUCKS = 100;
static constexpr int MEDIAN_N     = 5;  // must match Processing::MEDIAN_FRAMES

static Tuner         tuner(LABEL);
static ADCDriver     adc;
static IntervalTimer sampleTimer;

static int     pluckCount        = 0;
static float   centsSum          = 0.0f;
static int     within5           = 0;
static int     within10          = 0;
static bool    testDone          = false;
static bool    reported          = false;
static int16_t minP2PSinceReport = INT16_MAX;
static float   freqHistory[MEDIAN_N];
static int     histCount = 0;

// Insertion-sort median — mirrors Processing::medianFreq exactly
static float medianOf(float* arr, int n) {
  float s[MEDIAN_N];
  for (int i = 0; i < n; i++) s[i] = arr[i];
  for (int i = 1; i < n; i++) {
    float key = s[i];
    int   j   = i - 1;
    while (j >= 0 && s[j] > key) {
      s[j + 1] = s[j];
      j--;
    }
    s[j + 1] = key;
  }
  return s[n / 2];
}

static void sampleISR() {
  adc.startConversion();
  delayMicroseconds(5);
  int16_t buf[ADC_CH];
  adc.readChannels(buf, ADC_CH);
  tuner.addSample(buf[ADC_CH - 1]);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  Serial.println("========================================");
  Serial.print("  Accuracy Benchmark: ");
  Serial.println(LABEL);
  Serial.print("  Target : ");
  Serial.print(TARGET_HZ, 2);
  Serial.print(" Hz  |  Gate: ");
  Serial.print(GATE_MIN, 0);
  Serial.print(" - ");
  Serial.print(GATE_MAX, 0);
  Serial.println(" Hz");
  Serial.print("  Plucks : ");
  Serial.println(TOTAL_PLUCKS);
  Serial.print("  Pluck the ");
  Serial.print(LABEL);
  Serial.println(" string repeatedly.");
  Serial.println("========================================\n");

  adc.begin();
  sampleTimer.begin(sampleISR, 62.5f);
}

void loop() {
  if (testDone) return;
  if (!tuner.isReady()) return;
  sampleTimer.end();

  int16_t p2p = tuner.peakToPeak();

  if (p2p <= 200) {
    // Silence: fully reset
    reported          = false;
    minP2PSinceReport = INT16_MAX;
    tuner.reset();
    sampleTimer.begin(sampleISR, 62.5f);
    return;
  }

  if (reported) {
    // Onset detection: watch for a new pluck while string decays
    if (p2p < minP2PSinceReport) minP2PSinceReport = p2p;
    if (p2p > minP2PSinceReport * 1.5f) {
      reported          = false;
      minP2PSinceReport = INT16_MAX;
    }
    tuner.reset();
    sampleTimer.begin(sampleISR, 62.5f);
    return;
  }

  // Attempt pitch detection — accumulate MEDIAN_N frames before reporting
  tuner.removeDC();
  float freq = tuner.detectPitch(16000.0f);

  if (freq >= GATE_MIN && freq <= GATE_MAX) {
    freqHistory[histCount++] = freq;

    if (histCount < MEDIAN_N) {
      // Not enough frames yet — keep filling
      tuner.reset();
      sampleTimer.begin(sampleISR, 62.5f);
      return;
    }

    // Got MEDIAN_N valid readings — compute median and report
    float stableFreq  = medianOf(freqHistory, MEDIAN_N);
    histCount         = 0;
    reported          = true;
    minP2PSinceReport = INT16_MAX;

    float noteNumFloat = 69.0f + 12.0f * log2f(stableFreq / 440.0f);
    int   noteNum      = (int)(noteNumFloat + 0.5f);
    float cents        = (noteNumFloat - noteNum) * 100.0f;

    pluckCount++;
    centsSum += cents;
    if (fabsf(cents) <= 5.0f) within5++;
    if (fabsf(cents) <= 10.0f) within10++;

    Serial.print("#");
    Serial.print(pluckCount);
    Serial.print("\tcents: ");
    if (cents >= 0.0f) Serial.print("+");
    Serial.println(cents, 1);

    if (pluckCount >= TOTAL_PLUCKS) {
      testDone = true;
      sampleTimer.end();
      Serial.println();
      Serial.println("========================================");
      Serial.println("  RESULTS");
      Serial.println("========================================");
      float avg = centsSum / TOTAL_PLUCKS;
      Serial.print("  Avg cents off : ");
      if (avg >= 0.0f) Serial.print("+");
      Serial.println(avg, 2);
      Serial.print("  Within \xB15c  : ");
      Serial.print((within5 * 100) / TOTAL_PLUCKS);
      Serial.println("%");
      Serial.print("  Within \xB110c : ");
      Serial.print((within10 * 100) / TOTAL_PLUCKS);
      Serial.println("%");
      Serial.println("========================================");
      return;
    }
  }

  tuner.reset();
  sampleTimer.begin(sampleISR, 62.5f);
}

#endif  // ACCURACY_TEST

// -- MPM Accuracy Benchmark ----------------------------------------------------
// Identical to the YIN Accuracy Benchmark above but uses the McLeod Pitch
// Method (MPM / NSDF) algorithm instead.  Lets you compare the two algorithms
// side-by-side on the same hardware.
//
// Per-pluck output:  #N  freq: XX.XX Hz  cents: +/-X.X
// Summary:           avg cents, ±5c%, ±10c%
//
// Upload:  pio run -e teensy41_mpm_accuracy_lowe -t upload   (Low E)
//          pio run -e teensy41_mpm_accuracy_a    -t upload   (A)
//          pio run -e teensy41_mpm_accuracy_d    -t upload   (D)
//          pio run -e teensy41_mpm_accuracy_g    -t upload   (G)
//          pio run -e teensy41_mpm_accuracy_b    -t upload   (B)
//          pio run -e teensy41_mpm_accuracy_hie  -t upload   (High E)
// Monitor: pio device monitor

#ifdef ACCURACY_MPM_TEST

#include <Arduino.h>
#include <IntervalTimer.h>
#include <math.h>
#include "ADCDriver.h"
#include "Tuner.h"

#if ACCURACY_MPM_TEST == 1
static constexpr const char* LABEL     = "Low E";
static constexpr float       TARGET_HZ = 82.41f;
static constexpr float       GATE_MIN  = 50.0f;
static constexpr float       GATE_MAX  = 90.0f;
static constexpr uint8_t     ADC_CH    = 1;

#elif ACCURACY_MPM_TEST == 2
static constexpr const char* LABEL     = "A";
static constexpr float       TARGET_HZ = 110.0f;
static constexpr float       GATE_MIN  = 95.0f;
static constexpr float       GATE_MAX  = 135.0f;
static constexpr uint8_t     ADC_CH    = 2;

#elif ACCURACY_MPM_TEST == 3
static constexpr const char* LABEL     = "D";
static constexpr float       TARGET_HZ = 146.83f;
static constexpr float       GATE_MIN  = 120.0f;
static constexpr float       GATE_MAX  = 200.0f;
static constexpr uint8_t     ADC_CH    = 3;

#elif ACCURACY_MPM_TEST == 4
static constexpr const char* LABEL     = "G";
static constexpr float       TARGET_HZ = 196.0f;
static constexpr float       GATE_MIN  = 150.0f;
static constexpr float       GATE_MAX  = 250.0f;
static constexpr uint8_t     ADC_CH    = 4;

#elif ACCURACY_MPM_TEST == 5
static constexpr const char* LABEL     = "B";
static constexpr float       TARGET_HZ = 246.94f;
static constexpr float       GATE_MIN  = 200.0f;
static constexpr float       GATE_MAX  = 300.0f;
static constexpr uint8_t     ADC_CH    = 5;

#elif ACCURACY_MPM_TEST == 6
static constexpr const char* LABEL     = "High E";
static constexpr float       TARGET_HZ = 329.63f;
static constexpr float       GATE_MIN  = 300.0f;
static constexpr float       GATE_MAX  = 400.0f;
static constexpr uint8_t     ADC_CH    = 6;

#else
#error "ACCURACY_MPM_TEST must be 1-6 (1=Low E, 2=A, 3=D, 4=G, 5=B, 6=High E)"
#endif

static constexpr int MPM_TOTAL_PLUCKS = 100;
static constexpr int MPM_MEDIAN_N     = 5;  // must match Processing::MEDIAN_FRAMES

static Tuner         mpmTuner(LABEL);
static ADCDriver     mpmAdc;
static IntervalTimer mpmSampleTimer;

static int     mpmPluckCount        = 0;
static float   mpmCentsSum          = 0.0f;
static int     mpmWithin5           = 0;
static int     mpmWithin10          = 0;
static bool    mpmTestDone          = false;
static bool    mpmReported          = false;
static int16_t mpmMinP2PSinceReport = INT16_MAX;
static float   mpmFreqHistory[MPM_MEDIAN_N];
static int     mpmHistCount = 0;

// Insertion-sort median — mirrors Processing::medianFreq exactly
static float mpmMedianOf(float* arr, int n) {
  float s[MPM_MEDIAN_N];
  for (int i = 0; i < n; i++) s[i] = arr[i];
  for (int i = 1; i < n; i++) {
    float key = s[i];
    int   j   = i - 1;
    while (j >= 0 && s[j] > key) {
      s[j + 1] = s[j];
      j--;
    }
    s[j + 1] = key;
  }
  return s[n / 2];
}

static void mpmSampleISR() {
  mpmAdc.startConversion();
  delayMicroseconds(5);
  int16_t buf[ADC_CH];
  mpmAdc.readChannels(buf, ADC_CH);
  mpmTuner.addSample(buf[ADC_CH - 1]);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  Serial.println("========================================");
  Serial.print("  MPM Accuracy Benchmark: ");
  Serial.println(LABEL);
  Serial.print("  Target : ");
  Serial.print(TARGET_HZ, 2);
  Serial.print(" Hz  |  Gate: ");
  Serial.print(GATE_MIN, 0);
  Serial.print(" - ");
  Serial.print(GATE_MAX, 0);
  Serial.println(" Hz");
  Serial.print("  Plucks : ");
  Serial.println(MPM_TOTAL_PLUCKS);
  Serial.print("  Pluck the ");
  Serial.print(LABEL);
  Serial.println(" string repeatedly.");
  Serial.println("========================================\n");

  mpmAdc.begin();
  mpmSampleTimer.begin(mpmSampleISR, 62.5f);
}

void loop() {
  if (mpmTestDone) return;
  if (!mpmTuner.isReady()) return;
  mpmSampleTimer.end();

  int16_t p2p = mpmTuner.peakToPeak();

  if (p2p <= 200) {
    // Silence: fully reset
    mpmReported          = false;
    mpmMinP2PSinceReport = INT16_MAX;
    mpmTuner.reset();
    mpmSampleTimer.begin(mpmSampleISR, 62.5f);
    return;
  }

  if (mpmReported) {
    // Onset detection: watch for a new pluck while string decays
    if (p2p < mpmMinP2PSinceReport) mpmMinP2PSinceReport = p2p;
    if (p2p > mpmMinP2PSinceReport * 1.5f) {
      mpmReported          = false;
      mpmMinP2PSinceReport = INT16_MAX;
    }
    mpmTuner.reset();
    mpmSampleTimer.begin(mpmSampleISR, 62.5f);
    return;
  }

  // Attempt pitch detection — accumulate MPM_MEDIAN_N frames before reporting
  mpmTuner.removeDC();
  float freq = mpmTuner.detectPitchMPM(16000.0f);

  if (freq >= GATE_MIN && freq <= GATE_MAX) {
    mpmFreqHistory[mpmHistCount++] = freq;

    if (mpmHistCount < MPM_MEDIAN_N) {
      // Not enough frames yet — keep filling
      mpmTuner.reset();
      mpmSampleTimer.begin(mpmSampleISR, 62.5f);
      return;
    }

    // Got MPM_MEDIAN_N valid readings — compute median and report
    float stableFreq     = mpmMedianOf(mpmFreqHistory, MPM_MEDIAN_N);
    mpmHistCount         = 0;
    mpmReported          = true;
    mpmMinP2PSinceReport = INT16_MAX;

    float noteNumFloat = 69.0f + 12.0f * log2f(stableFreq / 440.0f);
    int   noteNum      = (int)(noteNumFloat + 0.5f);
    float cents        = (noteNumFloat - noteNum) * 100.0f;

    mpmPluckCount++;
    mpmCentsSum += cents;
    if (fabsf(cents) <= 5.0f) mpmWithin5++;
    if (fabsf(cents) <= 10.0f) mpmWithin10++;

    Serial.print("#");
    Serial.print(mpmPluckCount);
    Serial.print("\tfreq: ");
    Serial.print(stableFreq, 2);
    Serial.print(" Hz\tcents: ");
    if (cents >= 0.0f) Serial.print("+");
    Serial.println(cents, 1);

    if (mpmPluckCount >= MPM_TOTAL_PLUCKS) {
      mpmTestDone = true;
      mpmSampleTimer.end();
      Serial.println();
      Serial.println("========================================");
      Serial.println("  RESULTS (MPM)");
      Serial.println("========================================");
      float avg = mpmCentsSum / MPM_TOTAL_PLUCKS;
      Serial.print("  Avg cents off : ");
      if (avg >= 0.0f) Serial.print("+");
      Serial.println(avg, 2);
      Serial.print("  Within \xB15c  : ");
      Serial.print((mpmWithin5 * 100) / MPM_TOTAL_PLUCKS);
      Serial.println("%");
      Serial.print("  Within \xB110c : ");
      Serial.print((mpmWithin10 * 100) / MPM_TOTAL_PLUCKS);
      Serial.println("%");
      Serial.println("========================================");
      return;
    }
  }

  mpmTuner.reset();
  mpmSampleTimer.begin(mpmSampleISR, 62.5f);
}

#endif  // ACCURACY_MPM_TEST

// -- Motor Manual Test ---------------------------------------------------------
// Keyboard-controlled motor + optional auto-tune on pluck.  Designed to let
// you deliberately detune a string with the motor, pluck it, then watch the
// auto-tuner bring it back.
//
// Build envs: teensy41_manual_1 … teensy41_manual_6
// Upload:  pio run -e teensy41_manual_1 -t upload   (replace 1 with string #)
// Monitor: pio device monitor  (115200 baud, "No line ending" or "Newline")
//
// Key bindings typed in the Serial Monitor:
//   u       tune UP   — fine   (short pulse, slow speed)
//   U       tune UP   — coarse (long pulse, full speed)
//   d       tune DOWN — fine
//   D       tune DOWN — coarse
//   c       run UP continuously  (until SPACE / x)
//   C       run DOWN continuously
//   SPACE   STOP ALL MOTORS  ← emergency kill
//   x       STOP ALL MOTORS  ← alternate emergency kill
//   a       toggle auto-tune (tunes automatically after each pluck)
//   p       print current status
//   h / ?   print this help

#ifdef MOTOR_MANUAL_TEST

#include <Arduino.h>
#include <IntervalTimer.h>
#include "ADCDriver.h"
#include "MotorControl.h"
#include "Processing.h"
#include "Tuner.h"

// -- Per-channel configuration (mirrors TEST_CHANNEL and ACCURACY_TEST) -------
#if MOTOR_MANUAL_TEST == 1
static constexpr const char* LABEL     = "Low E";
static constexpr float       TARGET_HZ = 82.41f;
static constexpr float       GATE_MIN  = 50.0f;
static constexpr float       GATE_MAX  = 90.0f;
static constexpr uint8_t     ADC_CH    = 1;

#elif MOTOR_MANUAL_TEST == 2
static constexpr const char* LABEL     = "A";
static constexpr float       TARGET_HZ = 110.0f;
static constexpr float       GATE_MIN  = 95.0f;
static constexpr float       GATE_MAX  = 135.0f;
static constexpr uint8_t     ADC_CH    = 2;

#elif MOTOR_MANUAL_TEST == 3
static constexpr const char* LABEL     = "D";
static constexpr float       TARGET_HZ = 146.83f;
static constexpr float       GATE_MIN  = 120.0f;
static constexpr float       GATE_MAX  = 165.0f;
static constexpr uint8_t     ADC_CH    = ;

#elif MOTOR_MANUAL_TEST == 4
static constexpr const char* LABEL     = "G";
static constexpr float       TARGET_HZ = 196.0f;
static constexpr float       GATE_MIN  = 170.0f;
static constexpr float       GATE_MAX  = 220.0f;
static constexpr uint8_t     ADC_CH    = 4;

#elif MOTOR_MANUAL_TEST == 5
static constexpr const char* LABEL     = "B";
static constexpr float       TARGET_HZ = 246.94f;
static constexpr float       GATE_MIN  = 200.0f;
static constexpr float       GATE_MAX  = 300.0f;
static constexpr uint8_t     ADC_CH    = 4;

#elif MOTOR_MANUAL_TEST == 6
static constexpr const char* LABEL     = "High E";
static constexpr float       TARGET_HZ = 329.63f;
static constexpr float       GATE_MIN  = 300.0f;
static constexpr float       GATE_MAX  = 400.0f;
static constexpr uint8_t     ADC_CH    = 4;

#else
#error "MOTOR_MANUAL_TEST must be 1-6 (1=Low E, 2=A, 3=D, 4=G, 5=B, 6=High E)"
#endif

// Motor channel index matching MOTOR_PINS[] in MotorControl.h
static constexpr int MOTOR_CH = MOTOR_MANUAL_TEST - 1;

// -- Motor pulse constants ----------------------------------------------------
static constexpr int FINE_SPEED      = 175;  // matches MotorControl::MIN_SPEED
static constexpr int COARSE_SPEED    = 250;  // matches MotorControl::MAX_SPEED
static constexpr int FINE_PULSE_MS   = 80;
static constexpr int COARSE_PULSE_MS = 400;

// -- Auto-tune timing (mirrors Processing::loopProcessHelper) -----------------
static constexpr float CENTS_MAX_CLAMP = 60.0f;
static constexpr int   MIN_SPINS_MS    = 150;
static constexpr int   MAX_SPINS_MS    = 600;
static constexpr int   SETTLE_MS       = 600;  // wait after motor stops before re-sampling

// -- Pitch detection constants ------------------------------------------------
static constexpr int MEDIAN_N = 5;

// -- State --------------------------------------------------------------------
static bool    autoTune           = false;
static bool    motorContinuous    = false;
static float   lastFreq           = 0.0f;
static int     histCount          = 0;
static bool    reported           = false;
static int16_t minP2PSinceReport  = INT16_MAX;
static float   freqHist[MEDIAN_N] = {};

// -- Hardware -----------------------------------------------------------------
static Tuner         tuner(LABEL);
static ADCDriver     adc;
static MotorControl  motor;
static IntervalTimer sampleTimer;

// -- ISR: 16 kHz sample tick (same structure as TEST_CHANNEL) -----------------
static void sampleISR() {
  adc.startConversion();
  delayMicroseconds(5);
  int16_t buf[ADC_CH];
  adc.readChannels(buf, ADC_CH);
  tuner.addSample(buf[ADC_CH - 1]);
}

// -- Helpers ------------------------------------------------------------------
static void startTimer() { sampleTimer.begin(sampleISR, 62.5f); }
static void stopTimer() { sampleTimer.end(); }

// Delay that polls Serial every iteration for an e-stop (' ' or 'x').
// Returns true the instant a stop key is received so the motor can be cut
// immediately — fixes the issue where blocking delay() made e-stop unresponsive.
static bool safeDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (Serial.available()) {
      char c = (char)Serial.read();
      if (c == ' ' || c == 'x') return true;
      // Other keys received during a motor run are discarded; re-send after stop.
    }
  }
  return false;
}

// Drive motor for a fixed duration. Uses safeDelay() so an e-stop key cuts
// power immediately even mid-pulse instead of waiting for the delay to expire.
static void motorPulse(bool up, int speed, int ms) {
  stopTimer();
  motor.driveRaw(up, speed, MOTOR_CH);
  bool estop = safeDelay(ms);
  motor.stopAllMotors();
  tuner.reset();  // discard samples accumulated during the pulse
  startTimer();
  if (estop) Serial.println(F("!! STOP -- motor halted mid-pulse"));
}

// Stop everything: motor + optional continuous-run cleanup.
static void emergencyStop() {
  motor.stopAllMotors();
  if (motorContinuous) {
    motorContinuous = false;
    tuner.reset();
    startTimer();
  }
  Serial.println(F("!! STOP — all motors halted"));
}

static void printHelp() {
  Serial.println(F("------------------------------------------"));
  Serial.println(F("  MOTOR MANUAL TEST — key bindings"));
  Serial.print(F("  String : "));
  Serial.println(LABEL);
  Serial.print(F("  Target : "));
  Serial.print(TARGET_HZ, 2);
  Serial.println(F(" Hz"));
  Serial.println(F("------------------------------------------"));
  Serial.println(F("  u       tune UP   (fine  ~80 ms, slow)"));
  Serial.println(F("  U       tune UP   (coarse ~400 ms, fast)"));
  Serial.println(F("  d       tune DOWN (fine)"));
  Serial.println(F("  D       tune DOWN (coarse)"));
  Serial.println(F("  c       run UP continuously  (stop: SPACE/x)"));
  Serial.println(F("  C       run DOWN continuously"));
  Serial.println(F("  SPACE   STOP ALL MOTORS  <<< emergency kill (+ Enter if needed)"));
  Serial.println(F("  x       STOP ALL MOTORS  <<< recommended: works without Enter"));
  Serial.println(F("  a       toggle auto-tune mode"));
  Serial.println(F("  p       print status"));
  Serial.println(F("  h / ?   print this help"));
  Serial.println(F("------------------------------------------"));
  Serial.println(F("  Workflow: press U/D to detune, pluck,"));
  Serial.println(F("  enable auto-tune (a), pluck again to tune."));
  Serial.println(F("------------------------------------------"));
}

static void printStatus() {
  Serial.print(F("[STATUS]  string="));
  Serial.print(LABEL);
  Serial.print(F("  target="));
  Serial.print(TARGET_HZ, 2);
  Serial.print(F(" Hz  |  auto-tune="));
  Serial.print(autoTune ? F("ON ") : F("OFF"));
  Serial.print(F("  |  motor="));
  Serial.print(motorContinuous ? F("RUNNING") : F("idle   "));
  if (lastFreq > 20.0f) {
    float cents = Processing::getCentsOffTarget(TARGET_HZ, lastFreq);
    Serial.print(F("  |  last pluck="));
    Serial.print(lastFreq, 2);
    Serial.print(F(" Hz  ("));
    if (cents >= 0.0f) Serial.print('+');
    Serial.print(cents, 1);
    Serial.print(F(" c)"));
  }
  Serial.println();
}

// Insertion-sort median — mirrors Processing::medianFreq
static float medianOfN(float* arr) {
  float s[MEDIAN_N];
  for (int i = 0; i < MEDIAN_N; i++) s[i] = arr[i];
  for (int i = 1; i < MEDIAN_N; i++) {
    float k = s[i];
    int   j = i - 1;
    while (j >= 0 && s[j] > k) {
      s[j + 1] = s[j];
      j--;
    }
    s[j + 1] = k;
  }
  return s[MEDIAN_N / 2];
}

static void handleSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    switch (c) {
      case 'u':
        Serial.println(F(">> tune UP (fine)"));
        motorPulse(true, FINE_SPEED, FINE_PULSE_MS);
        break;
      case 'U':
        Serial.println(F(">> tune UP (coarse)"));
        motorPulse(true, COARSE_SPEED, COARSE_PULSE_MS);
        break;
      case 'd':
        Serial.println(F(">> tune DOWN (fine)"));
        motorPulse(false, FINE_SPEED, FINE_PULSE_MS);
        break;
      case 'D':
        Serial.println(F(">> tune DOWN (coarse)"));
        motorPulse(false, COARSE_SPEED, COARSE_PULSE_MS);
        break;
      case 'c':
        Serial.println(F(">> CONTINUOUS UP -- press SPACE or x to stop"));
        motorContinuous = true;
        stopTimer();
        motor.driveRaw(true, COARSE_SPEED, MOTOR_CH);
        break;
      case 'C':
        Serial.println(F(">> CONTINUOUS DOWN -- press SPACE or x to stop"));
        motorContinuous = true;
        stopTimer();
        motor.driveRaw(false, COARSE_SPEED, MOTOR_CH);
        break;
      case ' ':
      case 'x':
        emergencyStop();
        break;
      case 'a':
        autoTune = !autoTune;
        Serial.print(F(">> Auto-tune "));
        Serial.println(autoTune ? F("ENABLED") : F("DISABLED"));
        break;
      case 'p':
        printStatus();
        break;
      case 'h':
      case '?':
        printHelp();
        break;
      default:
        break;  // ignore newlines, carriage returns, etc.
    }
  }
}

// -- setup / loop -------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  adc.begin();
  motor.setup();
  startTimer();

  Serial.println(F("=========================================="));
  Serial.print(F("  Motor Manual Test: "));
  Serial.println(LABEL);
  Serial.print(F("  Target : "));
  Serial.print(TARGET_HZ, 2);
  Serial.println(F(" Hz"));
  Serial.println(F("=========================================="));
  printHelp();
  printStatus();
}

void loop() {
  handleSerial();

  // While continuously running, skip pitch detection to avoid filling the
  // buffer with motor-vibration noise.
  if (motorContinuous) return;

  if (!tuner.isReady()) return;
  stopTimer();  // hold ISR while processing buffer

  int16_t p2p = tuner.peakToPeak();

  if (p2p <= 200) {
    // Silence — full reset of onset state
    histCount         = 0;
    reported          = false;
    minP2PSinceReport = INT16_MAX;
    tuner.reset();
    startTimer();
    return;
  }

  if (reported) {
    // Onset re-detection: wait for a fresh pluck (same logic as Processing)
    if (p2p < minP2PSinceReport) minP2PSinceReport = p2p;
    if (p2p > minP2PSinceReport * 1.5f) {
      histCount         = 0;
      reported          = false;
      minP2PSinceReport = INT16_MAX;
    }
    tuner.reset();
    startTimer();
    return;
  }

  // Detect pitch
  tuner.removeDC();
  float freq = tuner.detectPitch(16000.0f);

  if (freq > GATE_MIN && freq < GATE_MAX) {
    freqHist[histCount++] = freq;

    if (histCount < MEDIAN_N) {
      // Not enough frames yet — keep accumulating
      tuner.reset();
      startTimer();
      return;
    }

    // MEDIAN_N valid reads — report
    float stable      = medianOfN(freqHist);
    histCount         = 0;
    reported          = true;
    minP2PSinceReport = INT16_MAX;
    lastFreq          = stable;

    float cents    = Processing::getCentsOffTarget(TARGET_HZ, stable);
    float absCents = fabsf(cents);

    Serial.print(F("[PLUCK]  "));
    Serial.print(stable, 2);
    Serial.print(F(" Hz  "));
    Serial.print(Processing::getNoteName(stable));
    Serial.print(F("  |  "));
    if (cents >= 0.0f) Serial.print('+');
    Serial.print(cents, 1);
    Serial.print(F(" c from target"));

    if (autoTune) {
      if (absCents <= 4.0f) {
        Serial.println(F("  -> IN TUNE"));
      } else {
        // Spin duration: same formula as Processing::loopProcessHelper
        float t      = constrain((absCents - 2.0f) / (CENTS_MAX_CLAMP - 2.0f), 0.0f, 1.0f);
        int   spinMs = MIN_SPINS_MS + (int)(t * (MAX_SPINS_MS - MIN_SPINS_MS));
        Serial.print(F("  -> AUTO-TUNING ("));
        Serial.print(cents > 0.0f ? F("tune down") : F("tune up"));
        Serial.print(F(", "));
        Serial.print(spinMs);
        Serial.println(F(" ms)"));

        // Timer is already stopped above; run motor then settle.
        // safeDelay() replaces delay() so e-stop cuts the motor immediately.
        motor.tune(TARGET_HZ, stable, MOTOR_CH);
        bool estop = safeDelay(spinMs);
        motor.stopAllMotors();
        if (estop) {
          Serial.println(F("!! STOP -- motor halted mid-tune"));
        } else {
          safeDelay(SETTLE_MS);  // motor is off; e-stop during settle is harmless
        }

        // Allow an immediate fresh reading without re-plucking
        reported  = false;
        histCount = 0;
      }
    } else {
      // Manual mode: just report direction hint
      if (absCents <= 4.0f) {
        Serial.println(F("  (in tune)"));
      } else {
        Serial.println(cents > 0.0f ? F("  (needs tuning DOWN)") : F("  (needs tuning UP)"));
      }
    }
  }

  tuner.reset();
  startTimer();
}

#endif  // MOTOR_MANUAL_TEST

// 6-String Auto-Tuner w/ Alternate Tuning Selection
// Upload:  pio run -e teensy41_manual_all -t upload
// Monitor: pio device monitor
// Serial commands:
//   a       toggle auto-tune (ON by default)
//   t       show tuning menu; then type 1-4 to switch tuning
//   p       print status
//   h / ?   print this help
//   SPACE / x   STOP ALL MOTORS (emergency kill)

#ifdef MOTOR_MANUAL_ALL

#include <Arduino.h>
#include <IntervalTimer.h>
#include "ADCDriver.h"
#include "MotorControl.h"
#include "Processing.h"
#include "Tuner.h"

// Tuning table (mirrors Processing::_tunings[])
struct ManualAllTuning {
  const char* name;
  const char* notes[6];
  float       freqs[6];
};

static constexpr ManualAllTuning TUNINGS[] = {
    {"Standard", {"E", "A", "D", "G", "B", "E"},
        {82.41f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f}},
    {"Drop D", {"D", "A", "D", "G", "B", "E"}, {73.42f, 110.0f, 146.83f, 196.0f, 246.94f, 329.63f}},
    {"Open G", {"D", "G", "D", "G", "B", "D"},
        {73.42f, 98.00f, 146.83f, 196.0f, 246.94f, 73.42f * 2}},
    {"Open D", {"D", "A", "D", "F#", "A", "D"},
        {73.42f, 110.0f, 146.83f, 185.0f, 110.0f * 2, 73.42f * 2}},
};

static constexpr int NUM_TUNINGS = sizeof(TUNINGS) / sizeof(TUNINGS[0]);

// Per-string frequency gates (wide enough to cover all tunings above)

static constexpr float GATE_MIN[6] = {50.0f, 85.0f, 125.0f, 165.0f, 200.0f, 270.0f};
static constexpr float GATE_MAX[6] = {100.0f, 135.0f, 200.0f, 250.0f, 295.0f, 370.0f};

static constexpr const char* STRING_LABELS[6] = {"Low E", "A", "D", "G", "B", "High E"};

// Auto tune timing (mirrors Processing::loopProcessHelper)
static constexpr float CENTS_MAX_CLAMP = 50.0f;
static constexpr int   MIN_SPINS_MS    = 60;
static constexpr int   MAX_SPINS_MS    = 600;
static constexpr int   SETTLE_MS       = 600;

// Pitch detection
static constexpr int MEDIAN_N = 5;

// Per string onset/median state
struct StrState {
  float   freqHist[MEDIAN_N];
  int     histCount;
  bool    reported;
  int16_t minP2PSinceReport;
  float   lastFreq;
};
static StrState ss[6];

// Global mode state
static int  activeTuning          = 0;
static bool autoTune              = true;
static bool waitingForTuningInput = false;

// Hardware
// Declared individually (same style as Processing.cpp) then collected into a
// pointer array for indexed access in the loop.
static Tuner  tunerLowE("Low E"), tunerA("A"), tunerD("D");
static Tuner  tunerG("G"), tunerB("B"), tunerHiE("High E");
static Tuner* tuners[6] = {&tunerLowE, &tunerA, &tunerD, &tunerG, &tunerB, &tunerHiE};

static ADCDriver     adc;
static MotorControl  motor;
static IntervalTimer sampleTimer;

// -- ISR: 16 kHz (channel mapping mirrors Processing::sampleISR) ------------
static void sampleISR() {
  adc.startConversion();
  delayMicroseconds(5);
  int16_t buf[6];
  adc.readChannels(buf, 6);
  tunerLowE.addSample(buf[0]);
  tunerA.addSample(buf[2]);  // note: A is buf[2], D is buf[1]
  tunerD.addSample(buf[1]);
  tunerG.addSample(buf[3]);
  tunerB.addSample(buf[4]);
  tunerHiE.addSample(buf[5]);
}

static void startTimer() { sampleTimer.begin(sampleISR, 62.5f); }
static void stopTimer() { sampleTimer.end(); }

// Polls Serial every ms so SPACE/x cuts a motor run immediately.
// Returns true if a stop key was received.
static bool safeDelay(unsigned long ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) {
    if (Serial.available()) {
      char c = (char)Serial.read();
      if (c == ' ' || c == 'x') return true;
    }
  }
  return false;
}

static float medianOfN(float* arr) {
  float s[MEDIAN_N];
  for (int i = 0; i < MEDIAN_N; i++) s[i] = arr[i];
  for (int i = 1; i < MEDIAN_N; i++) {
    float k = s[i];
    int   j = i - 1;
    while (j >= 0 && s[j] > k) {
      s[j + 1] = s[j];
      j--;
    }
    s[j + 1] = k;
  }
  return s[MEDIAN_N / 2];
}

static void resetAllStringState() {
  for (int i = 0; i < 6; i++) {
    ss[i].histCount         = 0;
    ss[i].reported          = false;
    ss[i].minP2PSinceReport = INT16_MAX;
  }
}

// -- Serial UI --------------------------------------------------------------
static void printHelp() {
  Serial.println(F("================================================"));
  Serial.println(F("  MOTOR MANUAL ALL — 6-string auto-tuner"));
  Serial.println(F("================================================"));
  Serial.println(F("  a         toggle auto-tune (ON by default)"));
  Serial.println(F("  t         show tuning menu, then type 1-4"));
  Serial.println(F("  p         print status"));
  Serial.println(F("  h / ?     print this help"));
  Serial.println(F("  SPACE/x   STOP ALL MOTORS (emergency kill)"));
  Serial.println(F("================================================"));
  Serial.println(F("  Pluck any string — it is detected automatically,"));
  Serial.println(F("  tuned to the selected tuning, then waits."));
  Serial.println(F("================================================"));
}

static void printTuningMenu() {
  Serial.println(F("-- Select tuning (type number) --"));
  for (int i = 0; i < NUM_TUNINGS; i++) {
    Serial.print(i == activeTuning ? F("* ") : F("  "));
    Serial.print(i + 1);
    Serial.print(F(": "));
    Serial.print(TUNINGS[i].name);
    Serial.print(F("  ["));
    for (int s = 0; s < 6; s++) {
      Serial.print(TUNINGS[i].notes[s]);
      if (s < 5) Serial.print('/');
    }
    Serial.println(F("]"));
  }
  waitingForTuningInput = true;
}

static void printStatus() {
  Serial.print(F("[STATUS]  tuning="));
  Serial.print(TUNINGS[activeTuning].name);
  Serial.print(F("  auto-tune="));
  Serial.println(autoTune ? F("ON") : F("OFF"));
  for (int i = 0; i < 6; i++) {
    Serial.print(F("  "));
    Serial.print(STRING_LABELS[i]);
    Serial.print(F(": target="));
    Serial.print(TUNINGS[activeTuning].freqs[i], 2);
    Serial.print(F(" Hz ("));
    Serial.print(TUNINGS[activeTuning].notes[i]);
    Serial.print(F(")"));
    if (ss[i].lastFreq > 20.0f) {
      float cents = Processing::getCentsOffTarget(TUNINGS[activeTuning].freqs[i], ss[i].lastFreq);
      Serial.print(F("  last="));
      Serial.print(ss[i].lastFreq, 2);
      Serial.print(F(" Hz ("));
      if (cents >= 0.0f) Serial.print('+');
      Serial.print(cents, 1);
      Serial.print(F(" c)"));
    }
    Serial.println();
  }
}

static void handleSerial() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    // Tuning selection mode: only digits and stop keys are meaningful.
    if (waitingForTuningInput) {
      if (c >= '1' && c <= '0' + NUM_TUNINGS) {
        activeTuning          = c - '1';
        waitingForTuningInput = false;
        resetAllStringState();
        Serial.print(F(">> Tuning set to: "));
        Serial.println(TUNINGS[activeTuning].name);
      } else if (c == ' ' || c == 'x') {
        waitingForTuningInput = false;
        motor.stopAllMotors();
        Serial.println(F("!! STOP — tuning select cancelled, all motors halted"));
      }
      // ignore anything else (newlines, other keys)
      continue;
    }

    switch (c) {
      case 'a':
        autoTune = !autoTune;
        Serial.print(F(">> Auto-tune "));
        Serial.println(autoTune ? F("ENABLED") : F("DISABLED"));
        break;
      case 't':
        printTuningMenu();
        break;
      case 'p':
        printStatus();
        break;
      case 'h':
      case '?':
        printHelp();
        break;
      case ' ':
      case 'x':
        motor.stopAllMotors();
        Serial.println(F("!! STOP — all motors halted"));
        break;
      default:
        break;
    }
  }
}

// -- setup / loop -----------------------------------------------------------
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
  }

  resetAllStringState();
  for (int i = 0; i < 6; i++) ss[i].lastFreq = 0.0f;

  adc.begin();
  motor.setup();
  startTimer();

  Serial.println(F("================================================"));
  Serial.println(F("  6-String Auto-Tuner — all strings"));
  Serial.println(F("================================================"));
  printHelp();
  printTuningMenu();  // prompt for tuning on startup
}

void loop() {
  handleSerial();
  if (waitingForTuningInput) return;  // hold off detection until tuning is chosen

  bool anyReady = false;
  for (int i = 0; i < 6; i++)
    if (tuners[i]->isReady()) {
      anyReady = true;
      break;
    }
  if (!anyReady) return;

  stopTimer();

  for (int i = 0; i < 6; i++) {
    if (!tuners[i]->isReady()) continue;

    int16_t   p2p = tuners[i]->peakToPeak();
    StrState& s   = ss[i];

    if (p2p <= 200) {
      s.histCount         = 0;
      s.reported          = false;
      s.minP2PSinceReport = INT16_MAX;
      tuners[i]->reset();
      continue;
    }

    // Onset re-detection: same logic as MOTOR_MANUAL_TEST / Processing
    if (s.reported) {
      if (p2p < s.minP2PSinceReport) s.minP2PSinceReport = p2p;
      if (p2p > s.minP2PSinceReport * 1.5f) {
        s.histCount         = 0;
        s.reported          = false;
        s.minP2PSinceReport = INT16_MAX;
      }
      tuners[i]->reset();
      continue;
    }

    tuners[i]->removeDC();
    float freq = tuners[i]->detectPitch(16000.0f);
    float tgt  = TUNINGS[activeTuning].freqs[i];

    if (freq > GATE_MIN[i] && freq < GATE_MAX[i]) {
      s.freqHist[s.histCount++] = freq;

      if (s.histCount >= MEDIAN_N) {
        float stable        = medianOfN(s.freqHist);
        s.histCount         = 0;
        s.reported          = true;
        s.minP2PSinceReport = INT16_MAX;
        s.lastFreq          = stable;

        float cents    = Processing::getCentsOffTarget(tgt, stable);
        float absCents = fabsf(cents);

        Serial.print(F("[PLUCK]  "));
        Serial.print(STRING_LABELS[i]);
        Serial.print(F("  "));
        Serial.print(stable, 2);
        Serial.print(F(" Hz  "));
        Serial.print(Processing::getNoteName(stable));
        Serial.print(F("  |  "));
        if (cents >= 0.0f) Serial.print('+');
        Serial.print(cents, 1);
        Serial.print(F(" c  -> target "));
        Serial.print(tgt, 2);
        Serial.print(F(" Hz ("));
        Serial.print(TUNINGS[activeTuning].notes[i]);
        Serial.print(')');

        if (autoTune) {
          if (absCents <= 4.0f) {
            Serial.println(F("  IN TUNE"));
          } else {
            float t      = constrain((absCents - 2.0f) / (CENTS_MAX_CLAMP - 2.0f), 0.0f, 1.0f);
            int   spinMs = MIN_SPINS_MS + (int)(t * (MAX_SPINS_MS - MIN_SPINS_MS));
            Serial.print(F("  AUTO-TUNING ("));
            Serial.print(cents > 0.0f ? F("tune down") : F("tune up"));
            Serial.print(F(", "));
            Serial.print(spinMs);
            Serial.println(F(" ms)"));

            motor.tune(tgt, stable, i);
            bool estop = safeDelay(spinMs);
            motor.stopAllMotors();
            if (estop) {
              Serial.println(F("!! STOP -- motor halted mid-tune"));
            } else {
              safeDelay(SETTLE_MS);
            }
            // Allow fresh reading after motor settles without re-plucking
            s.reported  = false;
            s.histCount = 0;
          }
        } else {
          if (absCents <= 4.0f)
            Serial.println(F("  (in tune)"));
          else
            Serial.println(cents > 0.0f ? F("  (needs DOWN)") : F("  (needs UP)"));
        }
      }
    }
    tuners[i]->reset();
  }

  startTimer();
}
#endif  // MOTOR_MANUAL_ALL