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

void Display::drawSelectTuningScreen(
    int selectedIndex, const char* note, const char* type, bool isEditingField) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x14_tr);

  const int rowY[3] = {15, 35, 55};

  // Start
  if (selectedIndex == 0 && !isEditingField) {
    u8g2.drawBox(0, 2, 128, 16);
    u8g2.setDrawColor(0);
    u8g2.drawStr(5, rowY[0], "Start");
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(5, rowY[0], "Start");
  }

  // Note row
  if (selectedIndex == 1 && !isEditingField) {
    u8g2.drawBox(0, 22, 128, 16);
    u8g2.setDrawColor(0);
    u8g2.drawStr(5, rowY[1], "Note:");
    u8g2.drawStr(70, rowY[1], note);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(5, rowY[1], "Note:");
    bool showNote = true;
    if (selectedIndex == 1 && isEditingField) {
      showNote = ((millis() / 250) % 2) == 0;
    }
    if (showNote) {
      u8g2.drawStr(70, rowY[1], note);
    }
  }

  // Type row
  if (selectedIndex == 2 && !isEditingField) {
    u8g2.drawBox(0, 42, 128, 16);
    u8g2.setDrawColor(0);
    u8g2.drawStr(5, rowY[2], "Type:");
    u8g2.drawStr(70, rowY[2], type);
    u8g2.setDrawColor(1);
  } else {
    u8g2.drawStr(5, rowY[2], "Type:");
    bool showType = true;
    if (selectedIndex == 2 && isEditingField) {
      showType = ((millis() / 250) % 2) == 0;
    }
    if (showType) {
      u8g2.drawStr(70, rowY[2], type);
    }
  }

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