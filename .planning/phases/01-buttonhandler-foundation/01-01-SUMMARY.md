---
phase: 01-buttonhandler-foundation
plan: "01"
subsystem: firmware
tags: [bounce2, debounce, gpio, rp2040, platformio, c++]

# Dependency graph
requires: []
provides:
  - ButtonHandler class with INPUT_PULLUP init and non-blocking Bounce2 polling on GPIO14/GPIO15
  - Bounce2 library declared in platformio.ini lib_deps
affects:
  - 01-02 (Phase 1 Plan 2 — main.cpp wiring of ButtonHandler)
  - 02-buttonhandler-events (Phase 2 — event accessors built on top of poll())

# Tech tracking
tech-stack:
  added:
    - thomasfredericks/Bounce2@^2.71 (PlatformIO registry, verified legitimate)
  patterns:
    - Constructor-injection: store pins in constructor, hardware init deferred to begin()
    - Debug macro flag: DEBUG_BUTTONHANDLER=true for Phase 1 serial verification
    - "#pragma once header guard (project convention, not #ifndef)"
    - 2-space indent with same-line opening brace

key-files:
  created:
    - include/ButtonHandler.h
    - src/ButtonHandler.cpp
  modified:
    - platformio.ini

key-decisions:
  - "Used attach(pin, INPUT_PULLUP) one-call form — no separate pinMode() per Bounce2 recommendation"
  - "DEBUG_BUTTONHANDLER set true so Phase 1 hardware bring-up can verify clean LOW transitions via serial"
  - "Phase 1 exposes only begin()/poll() — no event accessors; Phase 2 will add button1Fell() etc."

patterns-established:
  - "Pattern 1: ButtonHandler constructor stores only pin numbers — no hardware work before setup() runs"
  - "Pattern 2: poll() calls update() exactly once per Bounce instance per loop() invocation"
  - "Pattern 3: begin() uses Bounce.attach(pin, INPUT_PULLUP) + interval(25) for both buttons"

requirements-completed: [BTN-01, BTN-02, BTN-03]

# Metrics
duration: 2min
completed: 2026-05-31
---

# Phase 01 Plan 01: ButtonHandler Foundation Summary

**Bounce2-backed ButtonHandler class wrapping GPIO14/GPIO15 with INPUT_PULLUP init and 25ms non-blocking debounce, plus Bounce2 library declared in platformio.ini**

## Performance

- **Duration:** ~2 min
- **Started:** 2026-05-31T05:16:05Z
- **Completed:** 2026-05-31T05:17:24Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments
- Declared `thomasfredericks/Bounce2@^2.71` in platformio.ini lib_deps, satisfying BTN-03
- Created `include/ButtonHandler.h` with #pragma once, constructor, begin(), poll() declarations and Bounce private members (BTN-01, D-04)
- Created `src/ButtonHandler.cpp` implementing constructor with member initializer list, begin() with attach(pin, INPUT_PULLUP)+interval(25) for both pins, poll() with update() calls and DEBUG_BUTTONHANDLER-gated serial diagnostics (BTN-01, BTN-02, D-05, D-06)

## Task Commits

Each task was committed atomically:

1. **Task 1: Add Bounce2 to platformio.ini lib_deps** - `d303ae6` (chore)
2. **Task 2: Create include/ButtonHandler.h** - `dd4bdce` (feat)
3. **Task 3: Create src/ButtonHandler.cpp** - `dba3224` (feat)

**Plan metadata:** (committed with SUMMARY)

## Files Created/Modified
- `platformio.ini` - Added `thomasfredericks/Bounce2@^2.71` as last active lib_deps entry
- `include/ButtonHandler.h` - ButtonHandler class declaration with #pragma once, Bounce2 include, public constructor/begin/poll, private _pin1/_pin2/_b1/_b2
- `src/ButtonHandler.cpp` - ButtonHandler implementation: member initializer constructor, begin() with INPUT_PULLUP attach + 25ms interval, poll() with update() and debug fell() diagnostics

## Decisions Made
- Used `Bounce.attach(pin, INPUT_PULLUP)` one-call form rather than separate `pinMode()` + `attach()` — per Bounce2 official docs, the two-argument form sets INPUT_PULLUP mode internally; no separate pinMode needed
- Set `DEBUG_BUTTONHANDLER true` — matches WebServer.cpp and WebSocketClient.cpp production state; enables serial output during Phase 1 hardware bring-up for BTN-01 success criterion verification
- Phase 1 header exposes only three public methods (constructor, begin, poll) — no event accessors; Phase 2 extends the class for start/stop/chord detection

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- ButtonHandler scaffold is ready for Plan 01-02 (main.cpp wiring: include, global declaration, begin() call in setup(), poll() as first loop() call)
- Phase 2 will add event accessor methods (button1Fell(), button2Held(), chord detection) on top of this foundation
- Threat T-01-03 (floating GPIO phantom press) is mitigated: INPUT_PULLUP set via attach() before first read, 25ms debounce filters boot transients

## Self-Check

- [x] `platformio.ini` contains `thomasfredericks/Bounce2@^2.71` — verified (grep count: 1)
- [x] `include/ButtonHandler.h` exists with #pragma once, no #ifndef — verified
- [x] `src/ButtonHandler.cpp` exists — verified
- [x] `attach(_pin1, INPUT_PULLUP)` count: 1 — verified
- [x] `attach(_pin2, INPUT_PULLUP)` count: 1 — verified
- [x] `interval(25)` count: 2 — verified
- [x] `delay()` count: 0 — verified
- [x] Commits d303ae6, dd4bdce, dba3224 exist in git log

## Self-Check: PASSED

---
*Phase: 01-buttonhandler-foundation*
*Completed: 2026-05-31*
