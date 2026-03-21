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
  static void drawTuningScreen(
      const char* currentNote, const char* targetNote, const char* currentString, const bool sharp);

  static void drawSelectTuningScreen(
      int selectedIndex, const char* note, const char* type, bool isEditingField);
};

#endif  // DISPLAY_H