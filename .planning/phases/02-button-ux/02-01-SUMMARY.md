---
phase: 02-button-ux
plan: "01"
subsystem: firmware
tags: [buttonhandler, bounce2, armed-state-machine, websocketclient, timer-control]

# Dependency graph
requires:
  - phase: 01-buttonhandler-foundation
    provides: ButtonHandler class scaffold with Bounce2 debounce, Phase 1 poll() with BTN1/BTN2 fell diagnostics
provides:
  - ButtonHandler armed state machine with isArmed(), startStopPressed(), stopResetPressed() public API
  - WebSocketClient emitTimerControl(const char* action) stub dispatching to _timer methods
affects: [02-02, 03-fighttimer-sync]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - One-shot flag pattern: reset _startStopPending/_stopResetPending at top of poll() before evaluation
    - Ordered arm evaluation: arm check before Button 1 routing prevents 400ms chord misfire
    - strcmp dispatch to _timer: emitTimerControl reuses handleTimerUpdate() dispatch pattern

key-files:
  created: []
  modified:
    - include/ButtonHandler.h
    - src/ButtonHandler.cpp
    - include/WebSocketClient.h
    - src/WebSocketClient.cpp

key-decisions:
  - "One-shot flags reset at top of poll() before any conditional — prevents double-fire if accessor called multiple times per cycle (T-02-01)"
  - "Arm check placed before _b1.fell() routing — prevents first chord at exactly 400ms from misfiring as start/stop (T-02-02)"
  - "emitTimerControl Phase 2 stub calls _timer directly; Phase 3 prepends Socket.IO emit without renaming (D-05)"
  - "DEBUG_BUTTONHANDLER set to false; Phase 2 diagnostic messages replace Phase 1 BTN1/BTN2 fell output (D-12)"

patterns-established:
  - "Ordered poll() evaluation: update() -> reset flags -> arm check -> disarm check -> B1 routing"
  - "emitTimerControl seam pattern: Phase 2 local stub, Phase 3 prepends network emit"

requirements-completed: [BTN-04, BTN-05, BTN-06]

# Metrics
duration: 5min
completed: 2026-05-31
---

# Phase 02 Plan 01: ButtonHandler Armed State Machine + emitTimerControl Stub Summary

**ButtonHandler gains armed state machine with three-method event API (isArmed/startStopPressed/stopResetPressed) backed by Bounce2 hold-duration detection; WebSocketClient gains emitTimerControl strcmp dispatch stub wired to _timer directly as Phase 3 seam**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-05-31T20:33:00Z
- **Completed:** 2026-05-31T20:38:04Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments
- ButtonHandler exposes full armed-state event API: isArmed() persistent, startStopPressed()/stopResetPressed() one-shot per poll cycle
- Armed state machine in poll() with correct ordering: arm check before B1 routing prevents chord timing edge case
- Phase 1 BTN1 fell / BTN2 fell diagnostics removed; replaced with Phase 2 state messages ARMED/DISARMED/START_STOP/STOP_RESET
- WebSocketClient emitTimerControl stub ready for Plan 02 main.cpp wiring and Plan 03 network upgrade

## Task Commits

Each task was committed atomically:

1. **Task 1: ButtonHandler state machine** - `d391684` (feat)
2. **Task 2: WebSocketClient emitTimerControl stub** - `9a14570` (feat)

## Files Created/Modified
- `include/ButtonHandler.h` - Added isArmed(), startStopPressed(), stopResetPressed() declarations; _armed, _startStopPending, _stopResetPending private members
- `src/ButtonHandler.cpp` - Rewrote poll() with ordered state machine; extended constructor init list; removed Phase 1 diagnostics; added three const accessor one-liners
- `include/WebSocketClient.h` - Added emitTimerControl(const char* action) declaration in public section after // Status block
- `src/WebSocketClient.cpp` - Added emitTimerControl() implementation at end of file using strcmp dispatch to _timer->start()/stop()/reset()

## Decisions Made
- One-shot flags (_startStopPending, _stopResetPending) stored as bool members reset at top of poll() — simpler than deriving from Bounce2 state at accessor call time, matches D-02 idiom
- DEBUG_BUTTONHANDLER set to false (not repurposed to true) — Phase 2 diagnostics are optional; leaving off reduces serial noise in production
- was_armed local variable added in poll() to enable ARMED/DISARMED transition detection for debug output without separate tracking member

## Deviations from Plan

None - plan executed exactly as written. Threat model mitigations T-02-01 (one-shot flag reset) and T-02-02 (arm check ordering) both implemented as specified.

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- Plan 02-02 can immediately consume ButtonHandler event API and emitTimerControl seam
- main.cpp loop() wiring (setArmed + event routing), TimerDisplay armed overlay (D-09 through D-11) are the remaining Phase 2 deliverables
- No blockers; all four contract methods (isArmed, startStopPressed, stopResetPressed, emitTimerControl) are live and build cleanly

## Self-Check: PASSED

- include/ButtonHandler.h exists with 3 public methods + 3 private members
- src/ButtonHandler.cpp has ordered poll() + 3 accessor one-liners
- include/WebSocketClient.h has emitTimerControl declaration
- src/WebSocketClient.cpp has emitTimerControl implementation
- pio run exits 0 (SUCCESS)
- No BTN1 fell / BTN2 fell in ButtonHandler.cpp
- Commits d391684 and 9a14570 confirmed in git log

---
*Phase: 02-button-ux*
*Completed: 2026-05-31*
