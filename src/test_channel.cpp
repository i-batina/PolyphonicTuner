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
static constexpr uint8_t     ADC_CH    = 2;  // CH2 = buf[1], matches Processing.cpp

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
static constexpr uint8_t     ADC_CH    = 2;

#elif ACCURACY_TEST == 4
static constexpr const char* LABEL     = "G";
static constexpr float       TARGET_HZ = 196.0f;
static constexpr float       GATE_MIN  = 150.0f;
static constexpr float       GATE_MAX  = 250.0f;
static constexpr uint8_t     ADC_CH    = 2;

#elif ACCURACY_TEST == 5
static constexpr const char* LABEL     = "B";
static constexpr float       TARGET_HZ = 246.94f;
static constexpr float       GATE_MIN  = 200.0f;
static constexpr float       GATE_MAX  = 300.0f;
static constexpr uint8_t     ADC_CH    = 2;

#elif ACCURACY_TEST == 6
static constexpr const char* LABEL     = "High E";
static constexpr float       TARGET_HZ = 329.63f;
static constexpr float       GATE_MIN  = 300.0f;
static constexpr float       GATE_MAX  = 400.0f;
static constexpr uint8_t     ADC_CH    = 2;

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