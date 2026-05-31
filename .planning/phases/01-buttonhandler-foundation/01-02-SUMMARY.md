---
phase: 01-buttonhandler-foundation
plan: "02"
subsystem: firmware
tags: [rp2040, platformio, c++, gpio, bounce2, main.cpp]

# Dependency graph
requires:
  - phase: 01-buttonhandler-foundation plan 01
    provides: ButtonHandler class with begin()/poll() and Bounce2 lib_deps entry
provides:
  - ButtonHandler wired into src/main.cpp with #define pin constants, file-scope global, begin() in setup(), poll()-first in loop()
  - Successful pio run build producing firmware.uf2 (BTN-03 confirmed)
affects:
  - 02-buttonhandler-events (Phase 2 — event accessors built on top of this wiring)

# Tech tracking
tech-stack:
  added: []
  patterns:
    - buttonHandler.poll() is first call in loop() — ensures button events are never starved by WebServer::handleClient() blocking
    - Button pin constants grouped in HARDWARE PIN CONFIGURATION block with "do not change" comment
    - File-scope stack-allocated global — ButtonHandler buttonHandler(BTN_1, BTN_2) — no heap allocation

key-files:
  created: []
  modified:
    - src/main.cpp

key-decisions:
  - "buttonHandler.poll() placed as absolute first call in loop() per BTN-02 starvation-prevention requirement"
  - "Stack-allocated global at file scope — no `new`, no pointer — satisfies D-03 and 264KB SRAM constraint"
  - "Button Init added as section 2 in setup() (before Ethernet) so INPUT_PULLUP and debounce are active before any network blocking"

patterns-established:
  - "Pattern: poll()-first ordering — buttonHandler.poll() precedes timerDisplay.update() and WebServer::handleClient() to guarantee sub-loop latency"
  - "Pattern: pin constants in HARDWARE PIN CONFIGURATION block with explicit 'do not change' comment for hardware-locked assignments"

requirements-completed: [BTN-01, BTN-02]

# Metrics
duration: 5min
completed: 2026-05-30
---

# Phase 01 Plan 02: ButtonHandler main.cpp Wiring Summary

**ButtonHandler wired into src/main.cpp with GPIO14/GPIO15 #defines, file-scope global, begin() in setup() section 2, and poll()-first loop(); pio run exits 0 producing firmware.uf2**

## Performance

- **Duration:** ~5 min
- **Started:** 2026-05-30T05:22:00Z
- **Completed:** 2026-05-30T05:27:00Z
- **Tasks:** 2
- **Files modified:** 1

## Accomplishments
- Added `#include "ButtonHandler.h"` to project headers block alongside other project headers (BTN-01, D-01)
- Added `#define BTN_1 14` / `#define BTN_2 15` in HARDWARE PIN CONFIGURATION block with "V2 PCB — fixed, do not change" comment (D-07)
- Declared `ButtonHandler buttonHandler(BTN_1, BTN_2)` as stack-allocated file-scope global in GLOBAL OBJECTS block (D-03)
- Added `buttonHandler.begin()` as setup() section 2 (renumbered Ethernet→3, Network→4, WebSocket→5, Settings→6) (BTN-01)
- Placed `buttonHandler.poll()` as first statement in loop(), removed stale `// 2. Normal Timer Update` comment (BTN-02)
- `pio run` exits 0, firmware.uf2 produced at 232640 bytes / 22.3% flash (BTN-03)

## Task Commits

Each task was committed atomically:

1. **Task 1: Wire ButtonHandler into src/main.cpp (BTN-01, BTN-02, D-03, D-07)** - `df2583b` (feat)
2. **Task 2: Build verification — pio run exits 0 (BTN-03)** - no separate commit (verification-only, no file changes)

**Plan metadata:** (committed with SUMMARY)

## Files Created/Modified
- `src/main.cpp` - Added ButtonHandler.h include, BTN_1/BTN_2 pin defines, buttonHandler global, begin() in setup(), poll() first in loop()

## Decisions Made
- `buttonHandler.poll()` placed as absolute first call in loop() to satisfy BTN-02's starvation-prevention requirement — WebServer::handleClient() can block; polling first guarantees every loop() iteration sees fresh button state before any blocking call
- Stack-allocated global `ButtonHandler buttonHandler(BTN_1, BTN_2)` at file scope (no `new`) — consistent with D-03 and the 264KB SRAM constraint; global lifetime avoids dangling-pointer risk
- Button Init inserted as setup() section 2, before Ethernet Hardware Init (section 3) — INPUT_PULLUP and 25ms debounce are armed before any network blocking code runs, eliminating T-01-03 floating GPIO window

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None. All four changes applied cleanly. Build succeeded first attempt with no compiler errors or warnings related to ButtonHandler or Bounce2.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness
- Phase 1 is fully complete: BTN-01 (begin() in setup()), BTN-02 (poll() first in loop()), BTN-03 (Bounce2 in lib_deps, pio run exits 0) all satisfied
- Phase 2 (02-buttonhandler-events) can now add event accessor methods (button1Fell(), button2Held(), chord detection) on top of the wired ButtonHandler foundation
- Hardware bring-up verification: flash firmware.uf2 to RP2040, open serial monitor at 115200 baud, press each button — DEBUG_BUTTONHANDLER output should show "B1 fell" / "B2 fell" on clean LOW transitions with no phantom events at boot

## Self-Check

- [x] `#include "ButtonHandler.h"` in src/main.cpp — grep count: 1
- [x] `BTN_1 14` in src/main.cpp — grep count: 1
- [x] `BTN_2 15` in src/main.cpp — grep count: 1
- [x] `ButtonHandler buttonHandler(BTN_1, BTN_2)` in src/main.cpp — grep count: 1
- [x] `buttonHandler.begin()` in src/main.cpp — grep count: 1
- [x] `buttonHandler.poll()` in src/main.cpp — grep count: 1
- [x] `pio run` exits 0 with SUCCESS message — verified
- [x] `.pio/build/pico/firmware.uf2` exists (489472 bytes) — verified
- [x] Commit df2583b exists in git log — verified

## Self-Check: PASSED

---
*Phase: 01-buttonhandler-foundation*
*Completed: 2026-05-30*
