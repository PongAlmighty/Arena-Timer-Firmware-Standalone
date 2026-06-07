# Arena Timer Integration Guide

A complete technical reference for writing programs that interface with Arena Timer.

---

## 1. Integration Model

Arena Timer supports three control paths:

1. **HTTP REST API (direct control)** — your app sends HTTP requests to the timer device.
2. **Incoming Socket.IO feed (FightTimer-style)** — Arena Timer acts as a Socket.IO *client* and connects out to your server, then consumes `timer_update` messages.
3. **Outgoing Socket.IO events (physical buttons)** — Arena Timer emits `timer_control` events *to* your server when a physical button is pressed. Your server should handle these to keep all connected clients in sync.

---

## 2. Device Discovery and Connection

| Method       | Value                  |
|--------------|------------------------|
| mDNS         | `http://arenatimer.local` |
| Static IP    | `10.0.0.21` (fallback if DHCP fails) |
| HTTP port    | `80` |

**Recommended discovery sequence:**
1. Try `arenatimer.local` via mDNS
2. Fall back to the configured or static IP
3. Perform a health check: `GET /api/network/status`

---

## 3. REST API Reference

### Base URL
```
http://<timer-host>
```

---

### `POST /api` — Timer commands

**Request body** (form-encoded):

| Field    | Values                              |
|----------|-------------------------------------|
| `action` | `start` · `pause` · `reset` · `flip` |

**Response** (JSON):
```json
{"status": "success", "message": "Timer started"}
```

**Error response:**
```json
{"status": "error", "message": "Unknown action: <value>"}
```

---

### `POST /api/settings` — Apply timer and display configuration

**Request body** (form-encoded):

| Field        | Type   | Description                                            |
|--------------|--------|--------------------------------------------------------|
| `duration`   | int    | Duration in seconds (e.g. `180` = 3 minutes)          |
| `font`       | int    | Font ID (see [Font ID table](#font-id-table))          |
| `spacing`    | int    | Extra pixels between characters                        |
| `brightness` | int    | Display brightness `0–255`                             |
| `thresholds` | string | Pipe-delimited `seconds:#RRGGBB` pairs (URL-encoded)   |
| `default`    | string | Default color as `#RRGGBB` (URL-encoded)               |

**Threshold format example** (before URL encoding):
```
120:#FFFF00|60:#FF0000
```
(meaning: yellow at ≤2 minutes, red at ≤1 minute)

**Full example curl:**
```bash
curl -X POST http://arenatimer.local/api/settings \
  -d "duration=180&font=4&spacing=3&brightness=255&thresholds=120:%23FFFF00%7C60:%23FF0000&default=%2300FF00"
```

**Response:**
- `200 OK` — body: `Settings saved`
- `500` — body: `Error saving settings`

---

### `GET /api/status` — Timer state

**Response:**
```json
{"isPaused": true}
```

| Field      | Type    | Description                              |
|------------|---------|------------------------------------------|
| `isPaused` | boolean | `true` if stopped after being started    |

---

### `GET /api/settings` — Current configuration

**Response:**
```json
{"fontId": 4, "spacing": 3, "brightness": 255, "duration": 180}
```

| Field        | Type | Description             |
|--------------|------|-------------------------|
| `fontId`     | int  | Active font (see table) |
| `spacing`    | int  | Letter spacing (pixels) |
| `brightness` | int  | Brightness `0–255`      |
| `duration`   | int  | Total duration (seconds)|

---

### `GET /api/thresholds` — Color thresholds

**Response:**
```json
{
  "thresholds": [
    {"seconds": 120, "color": "#FFFF00"},
    {"seconds": 60,  "color": "#FF0000"}
  ],
  "defaultColor": "#00FF00"
}
```

Thresholds are returned in descending order by `seconds`. The timer uses the color of the lowest threshold whose `seconds` value is ≥ the current remaining time. If no threshold matches, `defaultColor` is used.

---

### `GET /api/network/status` — Network info

**Response:**
```json
{"ip": "10.0.0.21"}
```

---

### WebSocket Connection Management

#### `POST /api/websocket/connect`

**Request body** (form-encoded):

| Field  | Default       | Description                       |
|--------|---------------|-----------------------------------|
| `host` | *(required)*  | IP or hostname of your WS server  |
| `port` | `8765`        | Port number                       |
| `path` | `/socket.io/` | WebSocket path                    |
| `ns`   | `/`           | Socket.IO namespace               |

**Namespace format (FightTimer):** Use `/timer1` through `/timer5` — e.g. `ns=/timer1`. Do **not** use `/timer/1` (slash-separated format is not supported).

**Response:**
```json
{"status": "success", "message": "Connecting..."}
```

The connection completes asynchronously. Poll `/api/websocket/status` to confirm.

---

#### `POST /api/websocket/disconnect`

No body required.

**Response:**
```json
{"status": "success", "message": "Disconnected"}
```

---

#### `GET /api/websocket/status`

**Response:**
```json
{"connected": true, "url": "ws://192.168.1.100:8765/socket.io/"}
```

---

## 4. Socket.IO Message Contract (Server → Timer)

Once Arena Timer connects to your Socket.IO endpoint, it parses incoming messages in any of the following forms:

### Form 1 — Socket.IO event array (standard)
```json
["timer_update", {"action": "start"}]
```
Sent as packet `42[...]` in Socket.IO EIO4 framing.

### Form 2 — Wrapped object
```json
{"timer_update": {"action": "reset", "minutes": 3, "seconds": 0}}
```

### Form 3 — Flat action object
```json
{"action": "stop"}
```

---

### Supported `action` values

| Action     | Fields                                    | Behavior                                                          |
|------------|-------------------------------------------|-------------------------------------------------------------------|
| `start`    | `is_heartbeat` (bool, optional)           | Starts or resumes the timer. Ignored if `is_heartbeat` is `true`. |
| `stop`     | *(none)*                                  | Pauses the timer                                                  |
| `reset`    | `minutes` (int), `seconds` (int), `is_initial_sync` (bool, optional) | Sets duration and resets timer to idle. Defaults: `minutes=3`, `seconds=0`. If `is_initial_sync` is `true`, the next `start` event is treated as the initial sync and skips any round-trip status request. |
| `settings` | `settings` (object)                       | Acknowledged but not currently applied to physical timer          |

---

### `timer_status` event (initial sync)

On initial connect, if the server sends a `timer_status` event, the timer will apply the authoritative server time:

```json
["timer_status", {"time_left": 180}]
```

| Field       | Type | Description                       |
|-------------|------|-----------------------------------|
| `time_left` | int  | Remaining time in seconds         |

The timer will set its duration, reset, and start immediately from `time_left`.

---

### Socket.IO handshake sequence

Arena Timer handles the Socket.IO EIO4 protocol directly over a plain WebSocket connection. The handshake your server will observe:

1. **Client → Server**: HTTP upgrade to `ws://<host>:<port>/socket.io/?EIO=4&transport=websocket`
2. **Server → Client**: `0{...}` (EIO OPEN packet with session info)
3. **Client → Server**: `40` or `40<namespace>,` (Socket.IO CONNECT to namespace)
4. **Server → Client**: `40` (CONNECT acknowledged)
5. **Keepalive**: Server sends `2` (PING); timer replies with `3` (PONG)

---

## 5. Socket.IO Message Contract (Timer → Server)

When a physical button triggers a timer action, Arena Timer emits a `timer_control` event to your server **before** applying the action locally.

### Event format
```json
["timer_control", {"action": "start|stop|reset"}]
```

Sent as a standard Socket.IO `42[...]` packet on the configured namespace.

### Supported actions emitted

| Action  | Trigger                                  |
|---------|------------------------------------------|
| `start` | Button 1 pressed while timer is stopped  |
| `stop`  | Button 1 pressed while timer is running  |
| `reset` | Button 1 pressed while Button 2 is held  |

**Important — echo suppression:** After emitting `timer_control`, the timer opens a 500ms suppression window for that action. If the server echoes the same action back as a `timer_update` within that window, the timer ignores it (prevents double-actuation). The window is one-shot and action-matched.

Your server should treat `timer_control` the same as any other client requesting a timer action — apply it and broadcast the resulting `timer_update` to all connected clients as usual.

---

## 6. Physical Button Behavior

Arena Timer has two buttons wired to the V2 PCB:

| Button   | GPIO | Role                        |
|----------|------|-----------------------------|
| Button 1 | 14   | Start / Stop (primary)      |
| Button 2 | 15   | Arm modifier (hold to arm)  |

### State machine

```
DISARMED (default)
  Button 1 pressed → emit timer_control("start") or timer_control("stop")
                     (toggles based on current run state)
  Button 2 held ≥400ms → enter ARMED state (display shows overlay)

ARMED (Button 2 held)
  Button 1 pressed → emit timer_control("stop") + timer_control("reset")
                     (chord: stop-and-reset)
  Button 2 released → return to DISARMED
```

The armed state shows a visual confirmation overlay on the display. A hold-to-confirm interaction prevents accidental resets during a match.

---

## 7. Recommended Client Workflow

### Direct REST control
```
1. Discover host (mDNS or static IP)
2. GET /api/settings          → read current config
3. GET /api/thresholds         → read current thresholds
4. POST /api/settings          → push desired config
5. POST /api     action=start  → start the timer
   POST /api     action=pause  → pause the timer
   POST /api     action=reset  → reset the timer
6. GET /api/status             → poll timer state
```

### FightTimer / Socket.IO integration
```
1. Arena Timer calls POST /api/websocket/connect with your server's host/port/ns
2. Arena Timer connects and subscribes to timer_update events
3. Your server sends timer_update events to control the timer
4. Your server receives timer_control events when physical buttons are pressed
5. Treat timer_control events like any client action — broadcast to all clients
```

---

## 8. Font ID Table

| ID | Font                       |
|----|----------------------------|
| 0  | Default (5×7 pixel, 2×)    |
| 1  | FreeSans 9pt               |
| 2  | FreeSans 12pt              |
| 3  | FreeSansBold 9pt           |
| 4  | **FreeSansBold 12pt** *(default)* |
| 5  | FreeMono 9pt               |
| 6  | FreeMono 12pt              |
| 7  | FreeMonoBold 9pt           |
| 8  | FreeMonoBold 12pt          |
| 9  | FreeSerif 9pt              |
| 10 | FreeSerif 12pt             |
| 11 | FreeSerifBold 9pt          |
| 12 | FreeSerifBold 12pt         |
| 13 | Org_01 (retro pixel, 3×)   |
| 14 | Picopixel (retro pixel, 3×)|
| 15 | TomThumb (retro pixel, 3×) |
| 16 | Aquire Regular 12pt        |
| 17 | AquireBold 12pt            |
| 18 | AquireLight 12pt           |

---

## 9. Reliability Notes

- **No authentication or TLS** is implemented in the firmware. Deploy on a trusted LAN or VLAN.
- Use **retry with backoff** for HTTP calls.
- Timer commands are idempotent at the device level (safe to resend).
- **Validate** state after each command with a follow-up `GET /api/status`.
- Connection settings (host, port, path, namespace) are persisted to EEPROM and restored on reboot. The timer will auto-reconnect on boot if a prior connection was configured.
- Auto-reconnect uses **exponential backoff**: 10s → 20s → 40s → 60s (capped), resetting on successful connection.
- **Namespace validation:** On boot, the timer validates the stored namespace against the allowed set (`/`, `/timer1`–`/timer5`). Malformed values are reset to `/` and re-saved automatically.

---

## 10. Quick Reference — cURL Cheatsheet

```bash
# Start timer
curl -X POST http://arenatimer.local/api -d "action=start"

# Pause timer
curl -X POST http://arenatimer.local/api -d "action=pause"

# Reset timer
curl -X POST http://arenatimer.local/api -d "action=reset"

# Flip display orientation
curl -X POST http://arenatimer.local/api -d "action=flip"

# Set 3-minute duration with yellow/red thresholds
curl -X POST http://arenatimer.local/api/settings \
  -d "duration=180&font=4&spacing=3&brightness=255&thresholds=120:%23FFFF00%7C60:%23FF0000&default=%2300FF00"

# Get timer status
curl http://arenatimer.local/api/status

# Get current settings
curl http://arenatimer.local/api/settings

# Get color thresholds
curl http://arenatimer.local/api/thresholds

# Get IP address
curl http://arenatimer.local/api/network/status

# Connect Arena Timer's Socket.IO client to a FightTimer server (timer slot 1)
curl -X POST http://arenatimer.local/api/websocket/connect \
  -d "host=192.168.1.100&port=8765&path=/socket.io/&ns=/timer1"

# Check WebSocket connection status
curl http://arenatimer.local/api/websocket/status

# Disconnect WebSocket
curl -X POST http://arenatimer.local/api/websocket/disconnect
```
