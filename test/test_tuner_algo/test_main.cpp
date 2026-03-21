// Algorithmic unit tests for the Tuner and Processing helpers.
//
// These run entirely on the Teensy using synthetic sine-wave data —
// no guitar input is needed.
//
// Run with:  pio test -e teensy41_algo
// PlatformIO uploads the binary, reads Unity results over serial,
// and prints a pass/fail summary.

#include <Arduino.h>
#include <unity.h>

#include "Processing.h"
#include "Tuner.h"

#include <math.h>

static constexpr float FS = 16000.0f;  // sample rate used throughout

// ── Helper: fill a Tuner buffer with a pure sine wave ────────────────────────
static void fillSine(Tuner& t, float freqHz, float amplitude = 20000.0f) {
  for (int i = 0; i < BUFFER_SIZE; i++) {
    float   v       = amplitude * sinf(2.0f * (float)M_PI * freqHz * i / FS);
    int32_t clamped = (v > 32767.0f) ? 32767 : (v < -32768.0f) ? -32768 : (int32_t)v;
    t.addSample((int16_t)clamped);
  }
}

// ══ Buffer state ══════════════════════════════════════════════════════════════

void test_not_ready_initially() {
  Tuner t("T");
  TEST_ASSERT_FALSE(t.isReady());
}

void test_ready_after_full_buffer() {
  Tuner t("T");
  for (int i = 0; i < BUFFER_SIZE; i++) t.addSample(0);
  TEST_ASSERT_TRUE(t.isReady());
}

void test_reset_clears_ready() {
  Tuner t("T");
  for (int i = 0; i < BUFFER_SIZE; i++) t.addSample(0);
  t.reset();
  TEST_ASSERT_FALSE(t.isReady());
}

// addSample must stop accepting once the buffer is full (no overrun)
void test_no_overrun_past_buffer_size() {
  Tuner t("T");
  for (int i = 0; i < BUFFER_SIZE + 64; i++) t.addSample(1000);
  TEST_ASSERT_TRUE(t.isReady());  // still ready, no crash / overrun
}

// ══ Silence detection ═════════════════════════════════════════════════════════

void test_silent_on_flatline() {
  Tuner t("T");
  for (int i = 0; i < BUFFER_SIZE; i++) t.addSample(0);
  TEST_ASSERT_TRUE(t.isSilent(500));
}

void test_active_with_large_sine() {
  Tuner t("T");
  fillSine(t, 110.0f, 20000.0f);
  TEST_ASSERT_FALSE(t.isSilent(500));
}

// ══ Pitch detection: CH1 — Low E (82.41 Hz) ══════════════════════════════════

void test_detect_lowE_82hz() {
  Tuner t("LowE");
  fillSine(t, 82.41f, 20000.0f);
  float hz = t.detectPitch(FS);
  Serial.print("[CH1 LowE] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~82.41)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 82.41f, hz);
}

// Slightly sharp (+5 Hz) — motor should drive down
void test_detect_lowE_sharp() {
  Tuner t("LowE_sharp");
  fillSine(t, 87.41f, 20000.0f);
  float hz = t.detectPitch(FS);
  Serial.print("[CH1 LowE+5Hz] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~87.41)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 87.41f, hz);
}

// Slightly flat (-5 Hz) — motor should drive up
void test_detect_lowE_flat() {
  Tuner t("LowE_flat");
  fillSine(t, 77.41f, 20000.0f);
  float hz = t.detectPitch(FS);
  Serial.print("[CH1 LowE-5Hz] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~77.41)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 77.41f, hz);
}

// ══ Pitch detection: CH2 — A (110.00 Hz) ═════════════════════════════════════

void test_detect_A_110hz() {
  Tuner t("A");
  fillSine(t, 110.0f, 20000.0f);
  float hz = t.detectPitch(FS);
  Serial.print("[CH2 A] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~110.00)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 110.0f, hz);
}

void test_detect_A_sharp() {
  Tuner t("A_sharp");
  fillSine(t, 115.0f, 20000.0f);
  float hz = t.detectPitch(FS);
  Serial.print("[CH2 A+5Hz] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~115.00)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 115.0f, hz);
}

void test_detect_A_flat() {
  Tuner t("A_flat");
  fillSine(t, 105.0f, 20000.0f);
  float hz = t.detectPitch(FS);
  Serial.print("[CH2 A-5Hz] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~105.00)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 105.0f, hz);
}

// ══ Note-name helper ══════════════════════════════════════════════════════════

void test_note_name_silence() {
  // Below 20 Hz threshold → display "---"
  String n = Processing::getNoteName(1.0f);
  TEST_ASSERT_EQUAL_STRING("---", n.c_str());
}

void test_note_name_lowE() {
  // 82.41 Hz = E2
  String n = Processing::getNoteName(82.41f);
  TEST_ASSERT_EQUAL_STRING("E2", n.c_str());
}

void test_note_name_A() {
  // 110.00 Hz = A2
  String n = Processing::getNoteName(110.0f);
  TEST_ASSERT_EQUAL_STRING("A2", n.c_str());
}

// ══ Cents helper ══════════════════════════════════════════════════════════════

void test_cents_zero_at_A4() {
  // A4 = 440 Hz → 0 cents off
  TEST_ASSERT_INT_WITHIN(1, 0, Processing::getCentsOff(440.0f));
}

void test_cents_lowE_in_tune() {
  // Concert E2 = 82.41 Hz → near 0 cents
  TEST_ASSERT_INT_WITHIN(3, 0, Processing::getCentsOff(82.41f));
}

void test_cents_A_in_tune() {
  // Concert A2 = 110.00 Hz → near 0 cents
  TEST_ASSERT_INT_WITHIN(3, 0, Processing::getCentsOff(110.0f));
}

// ══ Median filter ═════════════════════════════════════════════════════════════

void test_median_known_values() {
  // Sorted: {1, 1, 3, 4, 5} → median = 3
  float arr[5] = {3.0f, 1.0f, 4.0f, 1.0f, 5.0f};
  float med    = Processing::medianFreq(arr, 5);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, med);
}

void test_median_sorted_input() {
  float arr[5] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f};
  float med    = Processing::medianFreq(arr, 5);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 30.0f, med);
}

void test_median_single_element() {
  float arr[5] = {82.5f, 0.0f, 0.0f, 0.0f, 0.0f};
  float med    = Processing::medianFreq(arr, 1);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 82.5f, med);
}

// ══ MPM Pitch Detection ═══════════════════════════════════════════════════════
// Tests call detectPitchMPM() directly so they always run regardless
// of whether -D USE_MPM is present in the build flags.

void test_mpm_lowE_82hz() {
  Tuner t("LowE_MPM");
  fillSine(t, 82.41f, 20000.0f);
  float hz = t.detectPitchMPM(FS);
  Serial.print("[MPM LowE] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~82.41)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 82.41f, hz);
}

void test_mpm_lowE_sharp() {
  Tuner t("LowE_sharp_MPM");
  fillSine(t, 87.41f, 20000.0f);
  float hz = t.detectPitchMPM(FS);
  Serial.print("[MPM LowE+5Hz] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~87.41)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 87.41f, hz);
}

void test_mpm_lowE_flat() {
  Tuner t("LowE_flat_MPM");
  fillSine(t, 77.41f, 20000.0f);
  float hz = t.detectPitchMPM(FS);
  Serial.print("[MPM LowE-5Hz] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~77.41)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 77.41f, hz);
}

void test_mpm_A_110hz() {
  Tuner t("A_MPM");
  fillSine(t, 110.0f, 20000.0f);
  float hz = t.detectPitchMPM(FS);
  Serial.print("[MPM A] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~110.00)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 110.0f, hz);
}

void test_mpm_A_sharp() {
  Tuner t("A_sharp_MPM");
  fillSine(t, 115.0f, 20000.0f);
  float hz = t.detectPitchMPM(FS);
  Serial.print("[MPM A+5Hz] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~115.00)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 115.0f, hz);
}

void test_mpm_A_flat() {
  Tuner t("A_flat_MPM");
  fillSine(t, 105.0f, 20000.0f);
  float hz = t.detectPitchMPM(FS);
  Serial.print("[MPM A-5Hz] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (expected ~105.00)");
  TEST_ASSERT_FLOAT_WITHIN(3.0f, 105.0f, hz);
}

// Octave guard: MPM must not return the 2nd harmonic (~164.82 Hz) instead of
// the fundamental (82.41 Hz). The half-period NSDF peak sits well below the
// key threshold on a pure sine, so the first qualifying peak should be correct.
void test_mpm_octave_guard_lowE() {
  Tuner t("LowE_octave_MPM");
  fillSine(t, 82.41f, 20000.0f);
  float hz = t.detectPitchMPM(FS);
  Serial.print("[MPM octave guard] detected=");
  Serial.print(hz, 2);
  Serial.println(" Hz  (must be < 100 Hz, not ~164 Hz)");
  TEST_ASSERT_LESS_THAN(100.0f, hz);
}

// ── Entry point ───────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
  }  // wait for USB enumeration

  UNITY_BEGIN();

  // Buffer state
  RUN_TEST(test_not_ready_initially);
  RUN_TEST(test_ready_after_full_buffer);
  RUN_TEST(test_reset_clears_ready);
  RUN_TEST(test_no_overrun_past_buffer_size);

  // Silence detection
  RUN_TEST(test_silent_on_flatline);
  RUN_TEST(test_active_with_large_sine);

  // CH1 — Low E pitch detection
  RUN_TEST(test_detect_lowE_82hz);
  RUN_TEST(test_detect_lowE_sharp);
  RUN_TEST(test_detect_lowE_flat);

  // CH2 — A pitch detection
  RUN_TEST(test_detect_A_110hz);
  RUN_TEST(test_detect_A_sharp);
  RUN_TEST(test_detect_A_flat);

  // Note name helper
  RUN_TEST(test_note_name_silence);
  RUN_TEST(test_note_name_lowE);
  RUN_TEST(test_note_name_A);

  // Cents helper
  RUN_TEST(test_cents_zero_at_A4);
  RUN_TEST(test_cents_lowE_in_tune);
  RUN_TEST(test_cents_A_in_tune);

  // Median filter
  RUN_TEST(test_median_known_values);
  RUN_TEST(test_median_sorted_input);
  RUN_TEST(test_median_single_element);

  // MPM pitch detection (calls detectPitchMPM directly)
  RUN_TEST(test_mpm_lowE_82hz);
  RUN_TEST(test_mpm_lowE_sharp);
  RUN_TEST(test_mpm_lowE_flat);
  RUN_TEST(test_mpm_A_110hz);
  RUN_TEST(test_mpm_A_sharp);
  RUN_TEST(test_mpm_A_flat);
  RUN_TEST(test_mpm_octave_guard_lowE);

  UNITY_END();
}

void loop() {}
