---
phase: 02-button-ux
verified: 2026-05-31T00:00:00Z
status: human_needed
score: 11/11 must-haves verified
overrides_applied: 0
human_verification:
  - test: "Hold Button 2 for >= 400ms — confirm yellow 'STOP?' blinks on matrix. Release Button 2 without pressing Button 1 — confirm display returns to normal timer with no timer state change (time not reset, running state unchanged)."
    expected: "Yellow 'STOP?' blinks on matrix while Button 2 is held. On release, matrix returns to normal timer display. Timer state is identical before and after the hold-release cycle."
    why_human: "Requires physical hardware — GPIO input, Bounce2 timing, LED matrix output, and 400ms hold timing cannot be verified programmatically."
  - test: "While Button 2 is held (armed), press Button 1 — confirm timer stops and resets to RESET state regardless of whether it was RUNNING or STOPPED before the chord."
    expected: "Timer returns to RESET state. Matrix returns to normal timer display. Both RUNNING→RESET and STOPPED→RESET chord paths must be confirmed."
    why_human: "Requires physical hardware execution and observation of timer state transitions through the matrix display."
  - test: "Press Button 1 alone (no Button 2 held) in sequence: RESET → press → running clock. Press again → stopped (time preserved). Press again → resumes from where it stopped. Confirm Button 1 alone never resets the timer."
    expected: "First press: timer starts counting down. Second press: timer freezes at stopped time. Third press: timer resumes from frozen time. No reset occurs on any solo Button 1 press."
    why_human: "Toggle behavior and absence-of-reset require visual confirmation on hardware. isRunning()/isRunning() state transitions cannot be trace-verified without live execution."
---

# Phase 2: Button UX Verification Report

**Phase Goal:** A box ref can start, stop, and arm-then-reset the timer using the two physical buttons
**Verified:** 2026-05-31
**Status:** human_needed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | ButtonHandler exposes isArmed() (persistent), startStopPressed() (one-shot), stopResetPressed() (one-shot) | VERIFIED | `include/ButtonHandler.h` lines 23-30: all three public declarations with correct signatures and doxygen comments |
| 2 | Armed state is entered when Button 2 has been held LOW for >=400ms and is still held | VERIFIED | `src/ButtonHandler.cpp` line 38: `if (_b2.read() == LOW && _b2.currentDuration() >= 400) { _armed = true; }` |
| 3 | Armed state is cleared when Button 2 rises (_b2.rose()); releasing Button 2 alone causes no timer action | VERIFIED | `src/ButtonHandler.cpp` line 43-45: `if (_b2.rose()) { _armed = false; }` — no timer call on this path |
| 4 | Button 1 fell while disarmed sets _startStopPending=true; while armed sets _stopResetPending=true | VERIFIED | `src/ButtonHandler.cpp` lines 48-56: correct branching inside `if (_b1.fell())` after armed state is settled |
| 5 | One-shot flags are reset to false at the top of every poll() call before any evaluation | VERIFIED | `src/ButtonHandler.cpp` lines 33-34: `_startStopPending = false; _stopResetPending = false;` before all conditionals |
| 6 | Arm check evaluates before Button 1 routing inside poll() | VERIFIED | `src/ButtonHandler.cpp`: arm detection at lines 37-40, disarm at 43-45, B1 routing at 48-56 — order confirmed |
| 7 | Phase 1 BTN1/BTN2 fell diagnostics are removed from poll() | VERIFIED | `grep "BTN1 fell\|BTN2 fell" src/ButtonHandler.cpp` returns empty — Phase 1 strings absent |
| 8 | WebSocketClient declares and implements emitTimerControl(const char* action) | VERIFIED | `include/WebSocketClient.h` line 28: declaration present; `src/WebSocketClient.cpp` lines 477-488: implementation present |
| 9 | emitTimerControl dispatches to _timer->start() / _timer->stop() / _timer->reset() via strcmp | VERIFIED | `src/WebSocketClient.cpp` lines 478-487: three strcmp branches each calling the correct _timer method |
| 10 | TimerDisplay setArmed(bool) implemented with blink-state reset on both arm and disarm transitions; armed update() short-circuits to yellow STOP? at 1Hz | VERIFIED | `src/TimerDisplay.cpp` lines 218-250: setArmed() resets _blink_state=true on both transitions; update() if(_armed) block with color565(255,200,0), setCursor(17,12), print("STOP?"), return |
| 11 | main.cpp loop() wires setArmed, stopResetPressed, startStopPressed routing, correct call order, null guards | VERIFIED | `src/main.cpp` lines 131-157: poll→setArmed→stopResetPressed(stop+reset)→startStopPressed(toggle)→update→Ethernet.maintain→handleClient→wsClient->poll; all emitTimerControl calls inside if(wsClient) |

**Score:** 11/11 truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `include/ButtonHandler.h` | Armed state machine API: isArmed(), startStopPressed(), stopResetPressed() | VERIFIED | All three public accessors declared (line 23-30); private members _armed, _startStopPending, _stopResetPending at lines 37-39 |
| `src/ButtonHandler.cpp` | Armed state machine implementation with _b2.currentDuration() >= 400 | VERIFIED | Full state machine in poll() (lines 28-64); arm threshold at line 38; accessors as one-liners at lines 62-64 |
| `include/WebSocketClient.h` | emitTimerControl declaration | VERIFIED | Line 28: `void emitTimerControl(const char* action);` under Phase 2 comment |
| `src/WebSocketClient.cpp` | emitTimerControl with strcmp dispatch to _timer methods | VERIFIED | Lines 477-488: full implementation with start/stop/reset branches |
| `include/TimerDisplay.h` | setArmed(bool) declaration, _armed private member | VERIFIED | Lines 103-105: public setArmed declaration; line 143: `bool _armed;` in private section |
| `src/TimerDisplay.cpp` | setArmed() implementation + armed short-circuit in update() | VERIFIED | Lines 218-250: setArmed() with transition handling; update() armed block with color565(255,200,0), setCursor(17,12), print("STOP?"), return |
| `src/main.cpp` | Button event routing and armed display wiring in loop() | VERIFIED | Lines 131-157: complete wiring matching plan spec exactly |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `src/ButtonHandler.cpp poll()` | `_startStopPending / _stopResetPending flags` | bool flags set in poll(), read by accessors | VERIFIED | Lines 33-34 reset flags; lines 50,54 set them; lines 63-64 expose via const accessors |
| `src/WebSocketClient.cpp emitTimerControl()` | `_timer->start() / _timer->stop() / _timer->reset()` | strcmp dispatch on action string | VERIFIED | Lines 478,481,484: strcmp checks; lines 480,483,486: _timer method calls |
| `src/main.cpp loop()` | `timerDisplay.setArmed()` | buttonHandler.isArmed() return value | VERIFIED | Line 132: `timerDisplay.setArmed(buttonHandler.isArmed());` — exact pattern match |
| `src/TimerDisplay.cpp update()` | STOP? rendering block | `_armed` early-return guard | VERIFIED | Line 234: `if (_armed) {` opens block; line 249: `return;` exits before normal rendering |
| `src/main.cpp loop()` | `wsClient->emitTimerControl()` | startStopPressed() / stopResetPressed() one-shot flags | VERIFIED | Lines 134-148: four emitTimerControl calls, all inside if(wsClient) null guards |

### Data-Flow Trace (Level 4)

These artifacts render display output from hardware input rather than DB queries.

| Artifact | Data Variable | Source | Produces Real Data | Status |
|----------|---------------|--------|--------------------|--------|
| `src/TimerDisplay.cpp update()` | `_armed` | `setArmed(buttonHandler.isArmed())` in main.cpp loop | Yes — live GPIO state via Bounce2 poll | FLOWING |
| `src/main.cpp loop()` | `startStopPressed()` / `stopResetPressed()` | `ButtonHandler::poll()` Bounce2 edge detection | Yes — hardware falling-edge events | FLOWING |
| `src/WebSocketClient.cpp emitTimerControl()` | `action` string | compile-time literals from main.cpp callers | Yes — hardcoded literals, no empty paths | FLOWING |

### Behavioral Spot-Checks

This is an embedded firmware target (RP2040). No runnable CLI entry points exist for the target binary. Step 7b SKIPPED — no host-runnable entry points. Build verification substitutes:

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| Project builds without errors | `pio run` | `[SUCCESS] Took 1.06 seconds` | PASS |
| Phase 1 diagnostics removed | `grep "BTN1 fell\|BTN2 fell" src/ButtonHandler.cpp` | empty | PASS |
| Accessor declarations present (3) | `grep -c "isArmed\|startStopPressed\|stopResetPressed" include/ButtonHandler.h` | 3 | PASS |
| emitTimerControl in header | `grep -c "emitTimerControl" include/WebSocketClient.h` | 1 | PASS |
| setArmed wired in main.cpp | `grep -c "setArmed(buttonHandler.isArmed())" src/main.cpp` | 1 | PASS |
| emitTimerControl calls in main.cpp | `grep -c "emitTimerControl" src/main.cpp` | 4 (stop+reset chord + stop/start toggle) | PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|-------------|-------------|--------|---------|
| BTN-04 | 02-01, 02-02 | Button 1 solo press toggles running/stopped only; never resets | VERIFIED (automated) / NEEDS HUMAN (behavioral) | startStopPressed() routes to emitTimerControl("start") or emitTimerControl("stop") via isRunning() toggle in main.cpp; hardware confirmation deferred to human check |
| BTN-05 | 02-01, 02-02 | Button 2 held >=400ms enters armed; release disarms with no action | VERIFIED (automated) / NEEDS HUMAN (behavioral) | _b2.currentDuration()>=400 sets _armed; _b2.rose() clears it; no timer call on disarm path |
| BTN-06 | 02-01, 02-02 | Button 1 while armed → stop and reset | VERIFIED (automated) / NEEDS HUMAN (behavioral) | stopResetPressed() routes to emitTimerControl("stop") then emitTimerControl("reset") in main.cpp |
| BTN-07 | 02-02 | Armed display overrides to yellow "STOP?" blinking at 1Hz | VERIFIED (automated) / NEEDS HUMAN (visual) | color565(255,200,0), setCursor(17,12), print("STOP?"), 500ms blink toggle in TimerDisplay::update() armed block |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| None found | — | — | — | — |

No TBD, FIXME, XXX, placeholder strings, empty return stubs, or hardcoded empty data found in any of the four modified files.

### Human Verification Required

The automated code analysis confirms all implementation is present and wired correctly. The following hardware behaviors require physical verification on the V2 PCB:

#### 1. Armed display overlay (BTN-05, BTN-07)

**Test:** Hold Button 2 (GPIO15) for >= 400ms. Observe the 64x32 RGB matrix.
**Expected:** Yellow text "STOP?" appears and blinks at approximately 1Hz (500ms on / 500ms off). Release Button 2 without pressing Button 1. Expected: matrix immediately returns to the normal timer countdown display with no change to the running/stopped state or the displayed time.
**Why human:** Requires physical hardware — GPIO active-low, Bounce2 timing, LED matrix output, and the 400ms threshold cannot be simulated in a host build.

#### 2. Chord reset (BTN-06)

**Test:** With the timer running, hold Button 2 for >= 400ms (confirm STOP? appears), then press Button 1 while still holding Button 2.
**Expected:** Timer stops and resets to the configured duration (RESET state). Matrix returns to normal display. Repeat with timer stopped (not running) — same result expected.
**Why human:** Two-state coverage (RUNNING→RESET and STOPPED→RESET) requires hardware execution and visual confirmation of timer state.

#### 3. Start/stop toggle, no-reset guarantee (BTN-04)

**Test:** With timer in RESET state, press Button 1 alone. Press again. Press again. Press five or more times.
**Expected:** First press: timer starts counting down (RUNNING). Second press: timer freezes (STOPPED, time preserved). Third press: timer resumes (RUNNING from where it stopped). No press of Button 1 alone causes a reset — the displayed time never jumps back to the full configured duration.
**Why human:** Toggle logic and the negative assertion (never resets) require live observation. isRunning() branch logic is code-verified but the actual timer state machine behavior needs hardware confirmation.

### Gaps Summary

No automated gaps found. All 11 must-have truths are verified in the codebase. All four requirements (BTN-04/05/06/07) have complete implementations wired end-to-end. The three items in Human Verification Required above represent planned hardware confirmation deferred from the 02-02-PLAN.md task 2 human-check block — they are not defects found during verification.

---

_Verified: 2026-05-31_
_Verifier: Claude (gsd-verifier)_
