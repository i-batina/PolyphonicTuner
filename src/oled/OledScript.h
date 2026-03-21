#ifndef OLEDSCRIPT_H
#define OLEDSCRIPT_H

#include <Wire.h>
#include <U8g2lib.h>

class OledScript {
 public:
  static void setup();
  static void loop();

 private:
  static inline bool lastLeft   = HIGH;
  static inline bool lastRight  = HIGH;
  static inline bool lastSelect = HIGH;
  static inline bool lastBack   = HIGH;

  static constexpr unsigned char myImage[4] U8X8_PROGMEM = {0xFF, 0x81, 0x81, 0xFF};
};

#endif  // OLEDSCRIPT_H