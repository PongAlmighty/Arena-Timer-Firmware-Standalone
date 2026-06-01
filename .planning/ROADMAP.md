# Roadmap: Arena Timer Firmware — Local Control & Sync

## Overview

Three horizontal layers build the complete button-control and sync capability. Phase 1 establishes the hardware foundation — stable GPIO init and a debounced ButtonHandler class. Phase 2 adds the full button UX on top of that stable base — start/stop toggle, chord-armed state, and the "STOP?" matrix overlay. Phase 3 closes the loop with FightTimer — outbound emit, echo suppression, and the startup sync fix that eliminates the ~1s display offset.

## Phases

**Phase Numbering:**

- Integer phases (1, 2, 3): Planned milestone work
- Decimal phases (2.1, 2.2): Urgent insertions (marked with INSERTED)

Decimal phases appear between their surrounding integers in numeric order.

- [x] **Phase 1: ButtonHandler Foundation** - GPIO init, Bounce2 integration, ButtonHandler class scaffold (completed 2026-05-31)
- [x] **Phase 2: Button UX** - Start/stop toggle, chord gesture, armed display overlay (completed 2026-05-31)
- [x] **Phase 3: Socket.IO Integration** - emitTimerControl(), echo suppression, timer sync fix

## Phase Details

### Phase 1: ButtonHandler Foundation

**Goal**: A stable, debounced button hardware layer exists and can be extended with behavior
**Depends on**: Nothing (first phase)
**Requirements**: BTN-01, BTN-02, BTN-03
**Success Criteria** (what must be TRUE):

  1. GPIO14 and GPIO15 are initialized as INPUT_PULLUP — pressing either button while monitoring serial shows a clean LOW transition with no phantom events at boot
  2. Bounce2 polls both buttons as the first call in loop(); a rapid sequence of button presses produces exactly one falling-edge event per physical press (no chatter)
  3. Bounce2 library is declared in platformio.ini lib_deps and the project builds without error

**Plans**: 2 plans
Plans:
**Wave 1**

- [x] 01-01-PLAN.md — Bounce2 lib_deps + ButtonHandler class scaffold (header + implementation)

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 01-02-PLAN.md — Wire ButtonHandler into main.cpp + pio run build verification

### Phase 2: Button UX

**Goal**: A box ref can start, stop, and arm-then-reset the timer using the two physical buttons
**Depends on**: Phase 1
**Requirements**: BTN-04, BTN-05, BTN-06, BTN-07
**Success Criteria** (what must be TRUE):

  1. Pressing Button 1 alone toggles between running and stopped only: first press starts a RESET timer (→ RUNNING), second press stops the clock without resetting it (→ STOPPED), third press resumes (→ RUNNING). Button 1 alone never causes a reset under any circumstance — RESET state is only reachable via the chord (BTN-06)
  2. Holding Button 2 for ≥400ms shows yellow blinking "STOP?" on the matrix; releasing Button 2 without pressing Button 1 returns the display to normal with no timer state change
  3. While Button 2 is held (armed), pressing Button 1 stops and resets the timer regardless of whether it was running or stopped — matrix returns to RESET display

**Plans**: 2 plans
Plans:
**Wave 1**

- [x] 02-01-PLAN.md — ButtonHandler state machine (armed/disarmed, one-shot events) + WebSocketClient emitTimerControl stub

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 02-02-PLAN.md — TimerDisplay armed overlay (yellow STOP? blink) + main.cpp button event routing

### Phase 3: Socket.IO Integration

**Goal**: All button actions sync to FightTimer bidirectionally with no echo loops and no startup time offset
**Depends on**: Phase 2
**Requirements**: SYNC-01, SYNC-02, SYNC-03
**Success Criteria** (what must be TRUE):

  1. Pressing any button emits a timer_control Socket.IO event (action: start / stop / reset) to FightTimer before the local timer state changes — FightTimer's log shows the event arriving within one LAN RTT
  2. After a local button emit, the echoed timer_update from FightTimer does not re-trigger the local timer; the suppression window is consumed on first match and does not block a subsequent legitimate remote action
  3. When FightTimer sends timer_update {action: start}, the Arena Timer's displayed countdown matches FightTimer's actual remaining time within one LAN RTT — the ~1s startup offset is eliminated

**Plans**: 1 plan
Plans:
**Wave 1**

- [x] 03-01-PLAN.md — WebSocketClient.h private members + WebSocketClient.cpp SYNC-01/02/03 implementation

## Progress

**Execution Order:**
Phases execute in numeric order: 1 → 2 → 3

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. ButtonHandler Foundation | 2/2 | Complete    | 2026-05-31 |
| 2. Button UX | 2/2 | Complete   | 2026-05-31 |
| 3. Socket.IO Integration | 1/1 | Complete | 2026-05-31 |
