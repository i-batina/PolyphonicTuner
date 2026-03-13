// Channel diagnostic test — compiled only when TEST_CHANNEL is defined.
//
// Upload with:   pio run -e teensy41_ch1 -t upload   (or ch2)
// Monitor with:  pio device monitor
//
// Build envs defined in platformio.ini:
//   teensy41_ch1  →  CH1, Low E  (target 82.41 Hz)
//   teensy41_ch2  →  CH2, A      (target 110.00 Hz)

#ifdef TEST_CHANNEL

#include <Arduino.h>
#include <IntervalTimer.h>
#include "ADCDriver.h"
#include "Processing.h"
#include "Tuner.h"

// ── Per-channel configuration ─────────────────────────────────────────────────
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

#else
#error "TEST_CHANNEL must be 1 or 2"
#endif

// ── Hardware objects ──────────────────────────────────────────────────────────
static Tuner         tuner(LABEL);
static ADCDriver     adc;
static IntervalTimer sampleTimer;

// ── ISR: 16 kHz sample tick ───────────────────────────────────────────────────
// For CH2, both channels must be clocked out; we discard CH1 and keep CH2.
static void sampleISR() {
  adc.startConversion();
  delayMicroseconds(5);  // tCONV max 4.2 µs (AD7606)
  int16_t buf[ADC_CH];
  adc.readChannels(buf, ADC_CH);
  tuner.addSample(buf[ADC_CH - 1]);
}

// ── setup / loop ──────────────────────────────────────────────────────────────
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

// ── Channel Scanner ───────────────────────────────────────────────────────────
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
static constexpr int16_t LIVE_THRESH  = 500;

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