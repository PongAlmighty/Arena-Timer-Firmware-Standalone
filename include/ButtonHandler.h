/**
 * Header for debounced button input on GPIO14 and GPIO15
 */

#pragma once

#include <Arduino.h>
#include <Bounce2.h>

class ButtonHandler {
public:
  /// @brief Construct a new ButtonHandler object
  /// @param pin1 GPIO pin for button 1 (active-low, INPUT_PULLUP)
  /// @param pin2 GPIO pin for button 2 (active-low, INPUT_PULLUP)
  ButtonHandler(uint8_t pin1, uint8_t pin2);

  /// @brief Initialize GPIO pins and Bounce2 debounce objects. Call in setup()
  void begin();

  /// @brief Poll both buttons. Call as first statement in loop()
  void poll();

  /// @brief Returns true while Button 2 has been held >= 400ms (persistent state)
  bool isArmed() const;

  /// @brief One-shot: true once per poll() cycle when Button 1 fell while disarmed
  bool startStopPressed() const;

  /// @brief One-shot: true once per poll() cycle when Button 1 fell while armed
  bool stopResetPressed() const;

private:
  uint8_t _pin1;
  uint8_t _pin2;
  Bounce _b1;
  Bounce _b2;
  bool _armed;
  bool _startStopPending;
  bool _stopResetPending;
};
