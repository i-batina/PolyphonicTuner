#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>

extern U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2;

class Display {
 public:
  static void initDisplay();
  static void writeSmallText(int x_pos, int y_pos, const char* text);

  static void drawHomeScreen(int selectedIndex);
  static void drawSettingsScreen(int selectedIndex);

  // Tuning selection: shows 4 named tunings, selectedIndex = 0–3
  static void drawSelectTuningScreen(int selectedIndex);

  // Live tuning screen (shown while motor is running)
  static void drawTuningScreen(
      const char* currentNote, const char* targetNote, const char* currentString, bool sharp);

  // "Pluck Xth string" prompt (shown between motor runs)
  static void drawPluckPromptScreen(
      const char* ordinal, const char* strLabel, const char* noteName, float targetHz);

  // Shown when all 6 strings are in tune
  static void drawAllTunedScreen();
};

#endif  // DISPLAY_H