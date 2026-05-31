---
phase: 02-button-ux
plan: "02"
subsystem: firmware
tags: [timerdisplay, armed-overlay, button-routing, emittimercontrol, loop-wiring]

# Dependency graph
requires:
  - phase: 02-button-ux/02-01
    provides: ButtonHandler isArmed()/startStopPressed()/stopResetPressed() API, WebSocketClient emitTimerControl() stub

provides:
  - TimerDisplay setArmed(bool) with 1Hz yellow STOP? blink overlay when armed
  - main.cpp loop() wired with setArmed, stopResetPressed chord routing, startStopPressed toggle routing
  - Full Phase 2 button UX end-to-end: BTN-04, BTN-05, BTN-06, BTN-07 all active

affects: [03-fighttimer-sync]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - Armed short-circuit in update(): check _armed at top of update() before all other rendering logic
    - Transition-aware setArmed(): reset _blink_state on both arm and disarm transitions for clean visual handoff
    - loop() call order: poll -> setArmed -> chord check -> toggle check -> update (D-11 anti-pattern prevented)
    - stopResetPressed before startStopPressed in loop(): chord reset takes priority per BTN-06

key-files:
  created: []
  modified:
    - include/TimerDisplay.h
    - src/TimerDisplay.cpp
    - src/main.cpp

key-decisions:
  - "Armed short-circuit returns early from update() before expired/paused/running logic — one-exit-path design prevents stale frames (D-10)"
  - "setArmed() resets _blink_state=true on both arm AND disarm transitions — prevents half-second dark gap on first blink and ensures normal rendering resumes visibly (Pitfall 3)"
  - "stopResetPressed checked before startStopPressed in loop() — ensures chord reset cannot be shadowed by start/stop event"
  - "isRunning() as sole toggle condition in startStopPressed branch — idle and paused both route to start via else, matching BTN-04 spec"

patterns-established:
  - "Armed overlay short-circuit: if (_armed) { blink logic; return; } at top of update()"
  - "Transition-aware boolean setter: check old vs new value, reset derived state on edges, then store"

requirements-completed: [BTN-04, BTN-05, BTN-06, BTN-07]

# Metrics
duration: 10min
completed: 2026-05-31
---

# Phase 02 Plan 02: TimerDisplay Armed Overlay + main.cpp Button Event Routing Summary

**TimerDisplay gains 1Hz warm-yellow STOP? blink overlay via setArmed() + update() short-circuit; main.cpp loop() wired with setArmed and full button event routing through emitTimerControl() — completing all four Phase 2 BTN requirements**

## Performance

- **Duration:** ~10 min
- **Started:** 2026-05-31T20:40:00Z
- **Completed:** 2026-05-31T20:50:00Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments
- TimerDisplay.setArmed(bool) implemented with transition-aware blink state resets on both arm/disarm transitions
- update() gains armed short-circuit block at the top: 1Hz blink of warm yellow (255,200,0) "STOP?" at cursor(17,12) on 64x32 matrix
- main.cpp loop() reordered: setArmed immediately after poll(), stopResetPressed chord check before startStopPressed toggle, timerDisplay.update() after all button routing
- All emitTimerControl calls wrapped in if (wsClient) null-guard; no change to Ethernet or WebSocket polling order
- Full Phase 2 success criteria met: BTN-04 toggle, BTN-05 armed display, BTN-06 chord reset, BTN-07 yellow blink all active

## Task Commits

Each task was committed atomically:

1. **Task 1: TimerDisplay armed overlay** - `92e4019` (feat)
2. **Task 2: main.cpp button event routing** - `b32e037` (feat)

## Files Created/Modified
- `include/TimerDisplay.h` - Added void setArmed(bool armed) declaration (doxygen) + bool _armed private member
- `src/TimerDisplay.cpp` - Extended constructor init list with _armed(false); added setArmed() implementation; added armed short-circuit block in update() with color565(255,200,0), setCursor(17,12), "STOP?" print
- `src/main.cpp` - Replaced loop() body with Phase 2 wiring: setArmed after poll(), chord/toggle event routing, update() after routing

## Decisions Made
- setArmed() resets _blink_state=true on disarm transition (not just arm) — ensures normal timer rendering resumes fully visible with no half-cycle dark gap
- _matrix.setFont(NULL) explicit in armed block — prevents GFX custom font from distorting "STOP?" layout (Open Question 1 from RESEARCH.md resolved conservatively)
- stopResetPressed branch checked first in loop() — chord reset takes logical priority; prevents race between the two one-shot flags in the same poll cycle
- isRunning()-only toggle condition — else covers both idle and paused states, matching BTN-04 intent that paused → start is a resume

## Deviations from Plan

None - plan executed exactly as written. All threat model mitigations implemented: T-02-04 (setArmed before update), T-02-05 (blink state reset on disarm).

## Issues Encountered
None.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- All four Phase 2 button UX requirements (BTN-04/05/06/07) are fully active
- Phase 3 (03-fighttimer-sync) can consume emitTimerControl seam: prepend Socket.IO emit without renaming the method
- Hardware verification: hold Button 2 >= 400ms to see STOP? blink; release to return to normal; hold B2 + press B1 for chord reset; press B1 alone to toggle start/stop

## Self-Check: PASSED

- include/TimerDisplay.h: void setArmed(bool armed) present (grep count: 2 — declaration + doxygen reference)
- include/TimerDisplay.h: bool _armed private member present
- src/TimerDisplay.cpp: _armed(false) in constructor init list
- src/TimerDisplay.cpp: void TimerDisplay::setArmed(bool armed) implementation present
- src/TimerDisplay.cpp: color565(255, 200, 0) present in armed block
- src/TimerDisplay.cpp: setCursor(17, 12) present in armed block
- src/TimerDisplay.cpp: "STOP?" print present in armed block
- src/main.cpp: timerDisplay.setArmed(buttonHandler.isArmed()) present (count: 1)
- src/main.cpp: stopResetPressed present (count: 1)
- src/main.cpp: startStopPressed present (count: 1)
- src/main.cpp: emitTimerControl calls present (count: 4 — stop+reset in chord, stop+start in toggle)
- src/main.cpp: timerDisplay.getTimer().isRunning() as toggle condition
- pio run exits 0 (SUCCESS)
- Commits 92e4019 and b32e037 confirmed in git log

---
*Phase: 02-button-ux*
*Completed: 2026-05-31*
