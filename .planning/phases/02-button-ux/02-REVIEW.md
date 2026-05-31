---
phase: 02-button-ux
reviewed: 2026-05-31T00:00:00Z
depth: standard
files_reviewed: 7
files_reviewed_list:
  - include/ButtonHandler.h
  - src/ButtonHandler.cpp
  - include/WebSocketClient.h
  - src/WebSocketClient.cpp
  - include/TimerDisplay.h
  - src/TimerDisplay.cpp
  - src/main.cpp
findings:
  critical: 3
  warning: 4
  info: 3
  total: 10
status: issues_found
---

# Phase 02: Code Review Report

**Reviewed:** 2026-05-31
**Depth:** standard
**Files Reviewed:** 7
**Status:** issues_found

## Summary

Reviewed the Phase 2 button-UX implementation: ButtonHandler debounce state machine, TimerDisplay armed overlay, WebSocketClient emitTimerControl stub, and main.cpp wiring. The ButtonHandler logic and display overlay are structurally sound. Three blockers were found: an EEPROM buffer overwrite that corrupts memory when a stored string is exactly at the capped length, a hardcoded debug flag left `true` in production WebSocket code that floods Serial on every received message, and a missing `emitTimerControl("reset")` duration argument that resets to the wrong time when called from the button path. Four warnings cover a stop+reset ordering hazard, a stale `_b2.fell()` check in the DISARMED log condition, unused header members, and a layering violation where TimerDisplay directly includes Ethernet hardware headers.

---

## Critical Issues

### CR-01: EEPROM buf[len] write out-of-bounds when len == 127

**File:** `src/WebSocketClient.cpp:99-102` (host), `112-115` (path), `120-123` (namespace)

**Issue:** The read loop guards `i < 127` (host/path) or `i < 63` (namespace), but the null terminator is written as `buf[len]`, not `buf[i]` where `i` stopped. When the EEPROM-stored length byte is exactly 127 (host or path) or 63 (namespace), the loop reads 0 bytes (because `i < 127` is immediately false for `len == 127`), then writes `buf[127] = 0`, which is one past the end of the 128-byte stack buffer — a classic off-by-one stack corruption on RP2040.

Concrete scenario: a host string of exactly 100 characters is saved (saveSettings caps at 100, so `len == 100`). The loop copies bytes 0..99 fine. `buf[100] = 0` is safe. The real hazard is if corrupted EEPROM ever delivers a `len` value of 127 or higher. EEPROM.read() returns `uint8_t` (0-255), so any value ≥ 127 will write the null terminator past the buffer end.

**Fix:** Clamp the null write to the actual stop index, not the raw `len`:

```cpp
// Host
int len = EEPROM.read(1);
int actual = 0;
for (int i = 0; i < len && i < 127; i++) {
  buf[i] = EEPROM.read(2 + i);
  actual = i + 1;
}
buf[actual] = '\0';  // safe: actual is always 0..127 (within [128] buffer)

// Path (same pattern, cap 127)
// Namespace: cap to 63, buffer index always 0..63 (buf[128] is large enough)
```

Alternatively use `buf[min(len, 127)] = 0` making the bound explicit:

```cpp
buf[len < 127 ? len : 127] = '\0';
```

---

### CR-02: DEBUG_WEBSOCKET left `true` — verbose Serial flood in production

**File:** `src/WebSocketClient.cpp:4`

**Issue:** `#define DEBUG_WEBSOCKET true` — the comment on line 3 even admits "ENABLED temporarily for debugging". With this flag live, every received WebSocket message, every ping/pong, every disconnect event, and every JSON parse step prints to Serial. On an RP2040 with a connected display loop running at ~60 Hz and WebSocket traffic, this generates continuous Serial output that: (a) consumes significant CPU time, (b) fills any connected USB serial buffer, and (c) constitutes a regression from the intended production build. The code path covers `DEBUG_PRINTLN(data)` at line 332 which dumps the full raw payload of every inbound message.

**Fix:**

```cpp
// src/WebSocketClient.cpp line 4
#define DEBUG_WEBSOCKET false   // was: true
```

---

### CR-03: emitTimerControl("reset") does not set duration — resets to wrong time

**File:** `src/main.cpp:137`, `src/WebSocketClient.cpp:477-488`

**Issue:** When `stopResetPressed()` fires, main.cpp calls:
```cpp
wsClient->emitTimerControl("stop");
wsClient->emitTimerControl("reset");
```

`emitTimerControl("reset")` in WebSocketClient.cpp only calls `_timer->reset()` with no duration argument:
```cpp
} else if (strcmp(action, "reset") == 0) {
    _timer->reset();
}
```

Compare to the network path in `handleTimerUpdate` (lines 442-455), which first calls `_timer->setDuration(...)` with the server-provided minutes/seconds, then `_timer->reset()`. The local button path skips `setDuration`, so `reset()` restores the timer to whatever duration it was last given — which may be a partial or zero value if the timer has never been set via the network. After a cold boot where no server connection was made, the button reset will reset to 0:00 instead of the configured duration (e.g., 3:00).

**Fix:** `emitTimerControl` should read the timer's own current duration before resetting, ensuring idempotent local resets:

```cpp
} else if (strcmp(action, "reset") == 0) {
    // Preserve the current configured duration — do not wipe it
    Timer::Components dur = _timer->getDuration();
    _timer->setDuration(dur);   // no-op if already set, but makes intent explicit
    _timer->reset();
}
```

Or, if the design intent is "reset to the last set duration", verify that `Timer::reset()` already does this correctly and document the contract. If `Timer::reset()` already restores to the last `setDuration` value, CR-03 is a documentation gap rather than a behavior bug — but that contract is not visible from the reviewed files and must be confirmed.

---

## Warnings

### WR-01: stop() then reset() in same loop tick — ordering hazard

**File:** `src/main.cpp:136-138`

**Issue:**
```cpp
wsClient->emitTimerControl("stop");
wsClient->emitTimerControl("reset");
```

Both calls happen synchronously in the same loop() tick. `emitTimerControl("stop")` calls `_timer->stop()`. `emitTimerControl("reset")` calls `_timer->reset()`. If `Timer::reset()` internally also stops the timer (which is typical), the double-stop is harmless. But if `Timer::reset()` is supposed to restart from the set duration (common in countdown designs), calling `stop()` first and `reset()` second may leave the timer in the wrong state depending on state machine transitions.

More importantly, a future Phase 3 implementation is expected to emit Socket.IO events *and* call local. If stop+reset are emitted as two separate Socket.IO messages, the server may process them out of order or treat "stop" as a no-op if the timer is already stopped. Consider combining into a single `"stopreset"` action or making `reset()` imply `stop()` internally.

**Fix:** Consolidate to a single action that the Timer state machine handles atomically:

```cpp
if (buttonHandler.stopResetPressed()) {
  if (wsClient) {
    wsClient->emitTimerControl("stopreset"); // single atomic action
  }
}
```

And add `"stopreset"` handling in `emitTimerControl`.

---

### WR-02: DISARMED debug log condition uses stale Bounce2 `fell()` result

**File:** `src/ButtonHandler.cpp:59`

**Issue:**
```cpp
if (!_armed && was_armed && !_b1.fell()) { DEBUG_PRINTLN("DISARMED"); }
```

`_b1.fell()` on line 59 calls the Bounce2 `fell()` getter a second time after it was already consumed on line 48. Bounce2's `fell()` is edge-triggered — it returns `true` exactly once per `update()` call per falling edge. After line 48's `if (_b1.fell())` check fires, the internal flag is cleared. By line 59, `_b1.fell()` will always return `false`.

Result: the DISARMED log line is **never suppressed** when B1 and B2 are released simultaneously. The condition `!_b1.fell()` on line 59 is always true (because the flag was consumed), so DISARMED logs even in the simultaneous-press-release scenario the condition was trying to exclude.

**Fix:** Cache the B1 fell result at the top of poll():

```cpp
bool b1_fell = _b1.fell();
// ...
if (b1_fell) {
  if (_armed) { _stopResetPending = true; }
  else        { _startStopPending = true; }
}
// ...
if (!_armed && was_armed && !b1_fell) { DEBUG_PRINTLN("DISARMED"); }
```

---

### WR-03: Three unused private member variables in WebSocketClient

**File:** `include/WebSocketClient.h:50-52`

**Issue:** `_isSocketIO`, `_socketIOFallback`, and `_socketIOSessionId` are declared in the class but never assigned or read in `WebSocketClient.cpp`. The local variable `isSocketIO` (lowercase, stack-scoped) in `connect()` and `poll()` shadows the intent, but the member `_isSocketIO` is never written. On an embedded target this wastes a small amount of RAM (1 bool + 1 bool + String overhead ≈ 12+ bytes), and more importantly these ghost members mislead future maintainers about what state is being tracked.

**Fix:** Remove the three unused members from the header:

```cpp
// Remove these three lines from WebSocketClient.h:
bool _isSocketIO;
bool _socketIOFallback;
String _socketIOSessionId;
```

---

### WR-04: TimerDisplay includes Ethernet hardware header — layering violation

**File:** `src/TimerDisplay.cpp:7`

**Issue:** `#include <Ethernet_Generic.hpp>` is included directly in `TimerDisplay.cpp`. `TimerDisplay` is a display abstraction; it has no business depending on the Ethernet stack. The only consumer is `renderNetworkStatus()` (line 488), which calls `Ethernet.localIP()` and `Ethernet.linkStatus()`. This couples the display class to the network hardware, preventing unit testing or reuse on hardware without Ethernet, and creates a circular concern (display knows about network layer).

Additionally, `main.cpp` includes `<Ethernet_Generic.h>` (no `.hpp` extension) while `TimerDisplay.cpp` includes `<Ethernet_Generic.hpp>` — these may refer to the same file through include-guards, but the inconsistent include spelling is a fragile convention.

**Fix:** Move network status data into a struct passed into `renderNetworkStatus()`:

```cpp
// TimerDisplay.h
struct NetworkInfo {
  String ip;
  bool linkUp;
};
void renderNetworkStatus(const NetworkInfo& info);
```

Then remove `#include <Ethernet_Generic.hpp>` from `TimerDisplay.cpp`. Main.cpp fills the struct before calling the method.

---

## Info

### IN-01: `_color` member in TimerDisplay is written but never read

**File:** `include/TimerDisplay.h:131`, `src/TimerDisplay.cpp:54-56`

**Issue:** `_color` (uint16_t) is set by `setColor()` at line 54-56 and initialized in the constructor, but `getCurrentColor()` (lines 444-471) always computes the color from `_default_r/g/b` or thresholds — it never reads `_color`. `setColor()` appears in the public API but has no effect on the rendered output.

**Fix:** Either remove `_color` and make `setColor()` delegate to `setDefaultColor()`, or wire `_color` into `getCurrentColor()` as the fallback when no thresholds match. The current behavior silently discards calls to `setColor()`.

---

### IN-02: `wsClient` null-check repeated but initialization is unconditional

**File:** `src/main.cpp:135, 142, 154`

**Issue:** `wsClient` is assigned unconditionally at line 108 (`wsClient = new WebSocketClient(...)`). The repeated `if (wsClient)` guards in loop() can never be false unless `new` throws (which on bare-metal RP2040 without exceptions will hard-fault, not return nullptr). The guards are harmless but add visual noise and false implication that wsClient can be null in normal operation.

**Fix:** Either assert once in setup() or document why the null-check is retained as a defensive pattern.

---

### IN-03: `renderNetworkStatus()` is never called from main.cpp

**File:** `src/TimerDisplay.cpp:488`, `include/TimerDisplay.h:119`

**Issue:** `renderNetworkStatus()` is declared and implemented but no call site exists in the reviewed files. This is either dead code, or a planned feature that was declared prematurely. Combined with WR-04 (Ethernet coupling), the dead method pulls in the Ethernet dependency for no current benefit.

**Fix:** If this method is not wired into the button-hold network-view feature yet, remove or stub it and remove the `Ethernet_Generic.hpp` include until the feature is actually implemented.

---

_Reviewed: 2026-05-31_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
