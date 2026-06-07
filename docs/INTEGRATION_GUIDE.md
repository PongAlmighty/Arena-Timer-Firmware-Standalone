# Arena Timer Integration Guide

A complete technical reference for writing programs that interface with Arena Timer.

---

## 1. Integration Model

Arena Timer supports two control paths:

1. **HTTP REST API (direct control)** — your app sends HTTP requests to the timer device.
2. **Incoming WebSocket / Socket.IO feed (FightTimer-style)** — Arena Timer acts as a WebSocket *client* and connects out to your server, then consumes `timer_update` messages.

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

> **Notes:**
> - **`pause` is REST-only.** The equivalent over the WebSocket feed is `stop` (see §4). Sending `stop` here returns an `Unknown action` error; sending `pause` over the WebSocket feed is silently ignored. Use the verb that matches the path.
> - **`reset` here takes no parameters** — it resets the timer to the *currently configured* duration. To change the duration, use `POST /api/settings` (`duration`). (The WebSocket `reset` in §4 *does* accept `minutes`/`seconds`; the two paths differ.)
> - **`flip` is vestigial and currently a no-op.** It returns `{"status":"success","message":"Orientation flipped"}` but does not rotate the display — the underlying orientation call is not wired up. Do not rely on it.

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

> **Limitation (relevant to state sync):** `isPaused` is the *only* state exposed. Internally the timer has three states — running, paused, idle — but `isPaused` is only `true` in the paused state. **Running and idle both report `isPaused:false` and are indistinguishable** through this endpoint. The firmware also does **not** expose the current remaining/elapsed time over REST or WebSocket. True bidirectional sync therefore requires either a firmware enhancement (a richer status payload) or an external state manager that tracks commands it has issued.

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

## 4. WebSocket / Socket.IO Message Contract (Server → Timer)

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

| Action     | Fields                     | Behavior                                                          |
|------------|----------------------------|-------------------------------------------------------------------|
| `start`    | *(none)*                   | Starts or resumes the timer                                       |
| `stop`     | *(none)*                   | Pauses the timer                                                  |
| `reset`    | `minutes` (int), `seconds` (int) | Sets duration and resets timer to idle. Defaults: `minutes=3`, `seconds=0` |
| `settings` | `settings` (object)        | Acknowledged but not currently applied to physical timer          |

---

### Socket.IO handshake sequence

Arena Timer handles the Socket.IO EIO4 protocol directly over a plain WebSocket connection. The handshake your server will observe:

1. **Client → Server**: HTTP upgrade to `ws://<host>:<port>/socket.io/?EIO=4&transport=websocket`
2. **Server → Client**: `0{...}` (EIO OPEN packet with session info)
3. **Client → Server**: `40` or `40<namespace>,` (Socket.IO CONNECT to namespace)
4. **Server → Client**: `40` (CONNECT acknowledged)
5. **Keepalive**: Server sends `2` (PING); timer replies with `3` (PONG)

---

## 5. Recommended Client Workflow (Direct REST Control)

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

---

## 6. Font ID Table

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

## 7. Reliability Notes

- **No authentication or TLS** is implemented in the firmware. Deploy on a trusted LAN or VLAN.
- Use **retry with backoff** for HTTP calls.
- Timer commands are idempotent at the device level (safe to resend).
- **Validate** state after each command with a follow-up `GET /api/status`.
- The WebSocket client stores connection settings in EEPROM and auto-reconnects on boot.

---

## 8. Quick Reference — cURL Cheatsheet

```bash
# Start timer
curl -X POST http://arenatimer.local/api -d "action=start"

# Pause timer
curl -X POST http://arenatimer.local/api -d "action=pause"

# Reset timer (to currently configured duration; use /api/settings to change duration)
curl -X POST http://arenatimer.local/api -d "action=reset"

# Flip display orientation — NOTE: vestigial no-op, returns success but does nothing
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

# Connect Arena Timer's WebSocket client to a FightTimer server
curl -X POST http://arenatimer.local/api/websocket/connect \
  -d "host=192.168.1.100&port=8765&path=/socket.io/"

# Check WebSocket connection status
curl http://arenatimer.local/api/websocket/status

# Disconnect WebSocket
curl -X POST http://arenatimer.local/api/websocket/disconnect
```
