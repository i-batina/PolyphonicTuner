// Script Defines States/Screens and Handles State Indexing From Button Inputs

#include "display.h"
#include <U8g2lib.h>

const char* exampleReadNote = "B";

U8G2_SSD1309_128X64_NONAME0_F_HW_I2C u8g2(U8G2_R0);

void Display::initDisplay() { u8g2.begin(); }

void Display::writeSmallText(int x_pos, int y_pos, const char* text) {
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(x_pos, y_pos, text);  // u8g2.drawStr(Horizontal Pos, Vertical Pos, "Text")
  u8g2.sendBuffer();
}

void Display::drawHomeScreen(int selectedIndex) {
  const char* items[] = {"Tune", "Settings", "Motor Test"};

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tr);

  for (int i = 0; i < 3; i++) {
    if (i == selectedIndex) {
      u8g2.drawBox(0, i * 20 + 2, 128, 16);
      u8g2.setDrawColor(0);
      u8g2.drawStr(5, i * 20 + 15, items[i]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(5, i * 20 + 15, items[i]);
    }
  }

  u8g2.sendBuffer();
}

void Display::drawSelectTuningScreen(int selectedIndex) {
  static constexpr const char* TUNING_NAMES[] = {
      "E Std.    E A D G B e",
      "D Std.    D G C F A D",
      "Drop D    D A D G B e",
      "Open G    D G D G B D",
      "Open D    D A D F#A D",
      "E Flat    EbAbDbGbBbeb",
  };

  const int VISIBLE      = 4;
  const int TOTAL        = sizeof(TUNING_NAMES) / sizeof(TUNING_NAMES[0]);
  int       scrollOffset = (selectedIndex >= VISIBLE) ? selectedIndex - VISIBLE + 1 : 0;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 10, "Select Tuning:");

  for (int row = 0; row < VISIBLE; row++) {
    int i = row + scrollOffset;
    if (i >= TOTAL) break;
    int y = 22 + row * 12;
    if (i == selectedIndex) {
      u8g2.drawBox(0, y - 9, 128, 11);
      u8g2.setDrawColor(0);
      u8g2.drawStr(2, y, TUNING_NAMES[i]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(2, y, TUNING_NAMES[i]);
    }
  }
  u8g2.sendBuffer();
}

void Display::drawPluckPromptScreen(
    const char* ordinal, const char* strLabel, const char* noteName, float targetHz) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 10, "Pluck string:");

  // Large string label, e.g. 4th - D
  u8g2.setFont(u8g2_font_7x14_tr);
  char line[32];
  snprintf(line, sizeof(line), "%s  %s", ordinal, strLabel);
  u8g2.drawStr(0, 32, line);

  // Target note and Hz in small font
  u8g2.setFont(u8g2_font_6x10_tr);
  char hz[24];
  snprintf(hz, sizeof(hz), "Target: %s  %.1f Hz", noteName, targetHz);
  u8g2.drawStr(0, 50, hz);

  // Blinking arrow to indicate waiting
  if ((millis() / 500) % 2 == 0) u8g2.drawStr(110, 50, "<<");

  u8g2.sendBuffer();
}

void Display::drawAllTunedScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tr);
  u8g2.drawStr(14, 22, "All strings");
  u8g2.drawStr(26, 40, "in tune!");
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(20, 58, "Press back/select");
  u8g2.sendBuffer();
}

void Display::drawTuningScreen(
    const char* currentNote, const char* targetNote, const char* currentString, const bool sharp) {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.drawStr(0, 12, "TUNING...");

  u8g2.drawStr(0, 28, "String:");
  u8g2.drawStr(60, 28, currentString);

  u8g2.drawStr(0, 38, "Target:");
  u8g2.drawStr(60, 38, targetNote);

  u8g2.drawStr(0, 48, "Current:");
  u8g2.drawStr(60, 48, currentNote);

  if (sharp) {
    bool showSharp = ((millis() / 250) % 2) == 0;  // Constantly sets variable as false then true

    if (showSharp) {
      u8g2.drawStr(84, 50, "v");
    }
  } else {
    bool showSharp = ((millis() / 250) % 2) == 0;  // Constantly sets variable as false then true

    if (showSharp) {
      u8g2.drawStr(84, 50, "^");
    }
  }
  u8g2.sendBuffer();
}

void Display::drawSettingsScreen(int selectedIndex) {
  const char* items[] = {"Brightness", "Reset", "Exit"};

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tr);
  u8g2.drawStr(0, 12, "Settings");

  for (int i = 0; i < 3; i++) {
    if (i == selectedIndex) {
      u8g2.drawBox(0, i * 16 + 18, 128, 14);
      u8g2.setDrawColor(0);
      u8g2.drawStr(5, i * 16 + 29, items[i]);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawStr(5, i * 16 + 29, items[i]);
    }
  }

  u8g2.sendBuffer();
}

// if reset, reboot teensy
// void Display::ResetMCU (ResetPressed) {
// reset teensy
//}