#include "OledScript.h"
#include "logo.h"
#include "display.h"
#include "ui.h"
#include "../MotorControl.h"

static UI           ui;
extern MotorControl motor;

#define BTN_LEFT 20
#define BTN_RIGHT 21
#define BTN_SELECT 16
#define BTN_BACK 17

// u8g2_font_6x10_tr        // small
// u8g2_font_7x14_tr        // medium
// u8g2_font_ncenB08_tr     // medium bold
// u8g2_font_ncenB14_tr     // large bold
// u8g2_font_logisoso16_tr  // nice large numeric/text
// u8g2_font_logisoso24_tr  // very large

void OledScript::setup() {
  // Set Button States
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  Display::initDisplay();  // calls u8g2.begin() which initializes Wire
  Wire.setClock(400000);   // upgrade I2C bus speed after Wire is initialized

  ui.initUI();
}

void OledScript::loop() {
  u8g2.clearBuffer();

  // Button Debouncing and Reading.
  bool left   = digitalRead(BTN_LEFT);
  bool right  = digitalRead(BTN_RIGHT);
  bool select = digitalRead(BTN_SELECT);
  bool back   = digitalRead(BTN_BACK);

  // Left: cycle through menu (advance selection)
  if (lastLeft == HIGH && left == LOW) ui.handleRight();
  // Left-middle: enter / confirm
  if (lastSelect == HIGH && select == LOW) ui.handleSelect();
  // Right-middle: back / cancel
  if (lastBack == HIGH && back == LOW) ui.handleBack();
  // Right: emergency stop — halt all motors and exit tuning
  if (lastRight == HIGH && right == LOW) {
    motor.stopAllMotors();
    ui.stopTuning();
  }

  lastLeft   = left;
  lastRight  = right;
  lastSelect = select;
  lastBack   = back;

  ui.renderCurrentScreen();
  delay(20);
}