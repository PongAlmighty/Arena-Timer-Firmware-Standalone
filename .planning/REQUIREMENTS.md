# Requirements — Arena Timer Local Control & Sync

## v1 Requirements

### Button Hardware

- [x] **BTN-01**: GPIO14 and GPIO15 initialized as `INPUT_PULLUP` (active-low) in `setup()` — eliminates floating-pin phantom presses at boot
- [x] **BTN-02**: Button states polled via Bounce2 library; poll is the first call in `loop()` to prevent starvation during HTTP blocking
- [x] **BTN-03**: `thomasfredericks/Bounce2@^2.71` added to `platformio.ini` `lib_deps`; debounce interval set to 25ms

### Button UX

- [x] **BTN-04**: Button 1 (GPIO14) solo press toggles between running and stopped only: RESET → RUNNING on first press; RUNNING → STOPPED on press; STOPPED → RUNNING on press. Button 1 alone NEVER resets the timer — reset is only possible via the chord (BTN-06).
- [x] **BTN-05**: Button 2 (GPIO15) held for ≥400ms enters "armed" state; releasing Button 2 at any point disarms with no further action taken
- [x] **BTN-06**: Button 1 pressed while system is in armed state → stop & reset (timer returns to RESET state regardless of whether it was RUNNING or STOPPED)
- [x] **BTN-07**: While armed, display overrides to yellow "STOP?" text blinking at 1 Hz; display returns to normal timer on disarm or confirm

### FightTimer Integration

- [x] **SYNC-01**: Every button action (start, stop, reset) emits a `timer_control` Socket.IO event to FightTimer with the corresponding action string before (not after) local timer state change
- [x] **SYNC-02**: `handleTimerUpdate()` suppresses incoming `timer_update` for 500ms after any local button emit, matched by action type; suppression is one-shot (consumed on first match)
- [x] **SYNC-03**: On receiving `timer_update {action: start}`, Arena Timer sets its timer to FightTimer's actual remaining time — if `time_left` is present in the event payload use it directly; otherwise emit `request_timer_status` and apply the response

## v2 Requirements (Deferred)

- Long-press Button 1 to cycle through fight duration presets (e.g., 2 min / 3 min / 5 min) — useful if FightTimer is unavailable
- Display of current timer duration on boot/reset for operator confirmation
- Configurable debounce threshold via web UI (for different button hardware on future PCB revisions)

## Out of Scope

- Pause as a distinct state — FightTimer has no pause action; stop-without-reset is the equivalent; the firmware stop/resume behavior maps to FightTimer start/stop
- Button interrupt mode — RP2040 hardware interrupts during WebSocket DMA transfers introduce race conditions with no debugging path; polling is correct for this architecture
- Authentication/authorization on button actions — device is on an isolated arena LAN; unauthenticated control is consistent with the existing web API
- Display orientation flip implementation — separate concern tracked in CONCERNS.md; not related to button control

## Traceability

| REQ-ID | Phase | Status |
|--------|-------|--------|
| BTN-01 | Phase 1 | Complete |
| BTN-02 | Phase 1 | Complete |
| BTN-03 | Phase 1 | Complete |
| BTN-04 | Phase 2 | Complete |
| BTN-05 | Phase 2 | Complete |
| BTN-06 | Phase 2 | Complete |
| BTN-07 | Phase 2 | Complete |
| SYNC-01 | Phase 3 | Complete |
| SYNC-02 | Phase 3 | Complete |
| SYNC-03 | Phase 3 | Complete |

---
*Requirements defined: 2026-05-30*
*Traceability updated: 2026-05-31*
