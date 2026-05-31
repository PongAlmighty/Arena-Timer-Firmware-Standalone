/**
 * Source code for debounced button input on GPIO14 and GPIO15
 */

#include "ButtonHandler.h"
#include <Arduino.h>

#define DEBUG_BUTTONHANDLER false
#if DEBUG_BUTTONHANDLER
#define DEBUG_PRINT(x)   Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

ButtonHandler::ButtonHandler(uint8_t pin1, uint8_t pin2)
    : _pin1(pin1), _pin2(pin2), _armed(false),
      _startStopPending(false), _stopResetPending(false) {}

void ButtonHandler::begin() {
  _b1.attach(_pin1, INPUT_PULLUP);
  _b1.interval(25);
  _b2.attach(_pin2, INPUT_PULLUP);
  _b2.interval(25);
}

void ButtonHandler::poll() {
  _b1.update();
  _b2.update();

  // Reset one-shot flags at top of poll (guarantees single-fire-per-cycle)
  _startStopPending = false;
  _stopResetPending = false;

  // Armed detection: Button 2 held >= 400ms (MUST evaluate before B1 routing)
  bool was_armed = _armed;
  if (_b2.read() == LOW && _b2.currentDuration() >= 400) {
    _armed = true;
  }

  // Disarm on Button 2 release
  if (_b2.rose()) {
    _armed = false;
  }

  // Button 1 fell: route to correct one-shot flag based on current arm state
  if (_b1.fell()) {
    if (_armed) {
      _stopResetPending = true;
      DEBUG_PRINTLN("STOP_RESET");
    } else {
      _startStopPending = true;
      DEBUG_PRINTLN("START_STOP");
    }
  }

  if (_armed && !was_armed) { DEBUG_PRINTLN("ARMED"); }
  if (!_armed && was_armed && !_b1.fell()) { DEBUG_PRINTLN("DISARMED"); }
}

bool ButtonHandler::isArmed() const { return _armed; }
bool ButtonHandler::startStopPressed() const { return _startStopPending; }
bool ButtonHandler::stopResetPressed() const { return _stopResetPending; }
