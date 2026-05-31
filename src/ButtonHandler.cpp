/**
 * Source code for debounced button input on GPIO14 and GPIO15
 */

#include "ButtonHandler.h"
#include <Arduino.h>

#define DEBUG_BUTTONHANDLER true
#if DEBUG_BUTTONHANDLER
#define DEBUG_PRINT(x)   Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

ButtonHandler::ButtonHandler(uint8_t pin1, uint8_t pin2)
    : _pin1(pin1), _pin2(pin2) {}

void ButtonHandler::begin() {
  _b1.attach(_pin1, INPUT_PULLUP);
  _b1.interval(25);
  _b2.attach(_pin2, INPUT_PULLUP);
  _b2.interval(25);
}

void ButtonHandler::poll() {
  _b1.update();
  _b2.update();
  // Phase 1 diagnostic output — remove in Phase 2
  if (_b1.fell()) { DEBUG_PRINTLN("BTN1 fell"); }
  if (_b2.fell()) { DEBUG_PRINTLN("BTN2 fell"); }
}
