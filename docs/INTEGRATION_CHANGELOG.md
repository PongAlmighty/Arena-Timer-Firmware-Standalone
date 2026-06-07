# Arena Timer Integration — What's New

This document calls out the additions and corrections to the integration spec since the original guide was distributed. If you built against the earlier version, here's what to look for.

---

## 1. The Timer Now Talks Back — `timer_control` Events

**This is the biggest addition.** When a physical button on the timer is pressed, Arena Timer now emits a `timer_control` event to your Socket.IO server *before* applying the action locally.

```json
["timer_control", {"action": "start"}]
// or "stop" or "reset"
```

**What this means for your server:** You need to listen for `timer_control` events on each timer's namespace. Treat them the same as any other client requesting an action — apply the action and broadcast a `timer_update` to all connected clients as usual.

If your server doesn't handle `timer_control`, physical button presses will still work locally on the timer, but your other connected clients won't be notified.

---

## 2. Physical Buttons Exist

The original guide had no mention of hardware buttons. The V2 PCB has two:

| Button | GPIO | Normal press | Hold Button 2 first, then press Button 1 |
|--------|------|--------------|------------------------------------------|
| Button 1 | 14 | Start / Stop toggle | Stop + Reset (chord) |
| Button 2 | 15 | Hold ≥400ms to arm | — |

The display shows a visual overlay when Button 2 is held (armed state) to confirm the chord before committing.

---

## 3. Echo Suppression — Don't Double-Send

After the timer emits `timer_control`, it opens a **500ms suppression window** for that action. If your server echoes the same action back as a `timer_update` within that window, the timer ignores it.

This is automatic and you don't need to implement anything special — but it's worth knowing so you don't mistake a "missing" response for a bug. The timer already acted on the button press locally; the echo just gets dropped.

---

## 4. Namespace Format Correction

The earlier guide showed `/` as the default namespace, which is correct for a generic connection. However, if you're using FightTimer-style multi-timer routing, the correct namespace format is:

```
/timer1   /timer2   /timer3   /timer4   /timer5
```

**Not** `/timer/1` (slash-separated). The firmware validates stored namespaces on boot and rejects that format.

When calling `POST /api/websocket/connect`, pass `ns=/timer1` (or whichever slot).

---

## 5. New Socket.IO Events (Server → Timer)

Two event fields were not documented in the original guide:

### `is_heartbeat` on `start` events
If your server broadcasts periodic heartbeat `start` events to keep clients in sync, set `is_heartbeat: true`. The timer ignores these, preventing unintended restarts from background broadcasts.

```json
["timer_update", {"action": "start", "is_heartbeat": true}]
```

### `is_initial_sync` on `reset` events
When a timer first connects mid-match, send a `reset` with `is_initial_sync: true` followed by a `start`. This tells the timer the next `start` is the initial sync and skips an internal round-trip status request.

```json
["timer_update", {"action": "reset", "minutes": 2, "seconds": 30, "is_initial_sync": true}]
["timer_update", {"action": "start"}]
```

### `timer_status` event (authoritative sync)
If you emit a `timer_status` event, the timer will immediately apply the server's authoritative time:

```json
["timer_status", {"time_left": 150}]
```

The timer will set its duration to `time_left` seconds, reset, and start.

---

## 6. EEPROM Persistence & Auto-Reconnect

Connection settings (host, port, path, namespace) are saved to EEPROM on first connect and restored automatically on reboot. The timer will attempt to reconnect using exponential backoff (10s → 20s → 40s → 60s max). You don't need to re-send the connect command after a power cycle.
