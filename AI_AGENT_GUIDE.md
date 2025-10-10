# AI Agent Guide for Arena Timer Firmware

**Last Updated:** October 10, 2025  
**Target Platform:** Waveshare RP2040 Zero with 64x32 RGB LED Matrix  
**Purpose:** Quick onboarding guide for AI agents assisting with this codebase

---

## Table of Contents
1. [Project Overview](#project-overview)
2. [Hardware Architecture](#hardware-architecture)
3. [Software Architecture](#software-architecture)
4. [Key Files and Their Roles](#key-files-and-their-roles)
5. [Critical Design Patterns](#critical-design-patterns)
6. [Common Pitfalls and Solutions](#common-pitfalls-and-solutions)
7. [API Reference](#api-reference)
8. [WebSocket Integration](#websocket-integration)
9. [Build System](#build-system)
10. [Development Workflow](#development-workflow)

---

## Project Overview

### What This Is
An embedded firmware for a combat robotics arena timer that displays countdown/stopwatch on a 64x32 RGB LED matrix, controlled via web interface and WebSocket API for livestream integration.

### Key Features
- **Web-based control** via Ethernet (W5500 module)
- **LED Matrix display** with customizable fonts and colors
- **Color thresholds** - automatic color changes based on remaining time
- **WebSocket client** - sync with FightTimer livestream overlay
- **mDNS support** - accessible at `http://arenatimer.local`
- **Static IP configuration** - default `10.0.0.21`

---

## Hardware Architecture

### Microcontroller: Waveshare RP2040 Zero
- **CPU:** Dual-core ARM Cortex-M0+ @ 133MHz
- **RAM:** 256KB SRAM
- **Flash:** 2MB
- **GPIO:** Limited pins, carefully allocated

### Display: 64x32 RGB LED Matrix (P5 pitch)
- **Interface:** HUB75 parallel protocol
- **Driver:** Adafruit Protomatter library
- **Color Depth:** 16-bit RGB565
- **Default Orientation:** 180° rotation

### Network: Wiznet W5500 Ethernet Module
- **Interface:** SPI (Hardware SPI0)
- **Pin Mapping:**
  - CS: GPIO 1
  - SCK: GPIO 2
  - MOSI: GPIO 3
  - MISO: GPIO 4
- **Network Mode:** Static IP (no DHCP)
- **Default IP:** 10.0.0.177

---

## Software Architecture

### Component Overview
```
┌─────────────────────────────────────────────────────────────┐
│                         main.cpp                            │
│  - Setup: Initialize all components                         │
│  - Loop: Poll network, update timer, refresh display        │
└─────────────────────────────────────────────────────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
┌───────────────┐   ┌──────────────────┐   ┌────────────────┐
│  Comms.cpp    │   │ TimerDisplay.cpp │   │ WebSocketClient│
│               │   │                  │   │                │
│ - HTTP Server │   │ - Render timer   │   │ - FightTimer   │
│ - REST API    │   │ - Format text    │   │   integration  │
│ - Web UI HTML │   │ - Color control  │   │ - Event parser │
│ - mDNS        │   │ - Font switching │   │ - Auto-reconnect│
└───────────────┘   └──────────────────┘   └────────────────┘
        │                     │
        │                     ▼
        │           ┌──────────────────┐
        │           │    Timer.cpp     │
        │           │                  │
        │           │ - State machine  │
        │           │ - Time calc      │
        └──────────▶│ - Blink control  │
                    └──────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  RGBMatrix.cpp   │
                    │                  │
                    │ - Matrix init    │
                    │ - Protomatter    │
                    │ - Low-level draw │
                    └──────────────────┘
```

---

## Key Files and Their Roles

### `src/main.cpp`
**Purpose:** Application entry point and main loop

**Key Responsibilities:**
- Initialize Ethernet with static IP
- Start mDNS service (`arenatimer.local`)
- Create Timer, TimerDisplay, Comms, and WebSocketClient instances
- Main loop: `Ethernet.maintain()`, `wsClient->poll()`, `timerDisplay.update()`

**Important Variables:**
- `static_ip[]` - Change this to set device IP address
- `mac[]` - Ethernet MAC address
- `hostname` - mDNS hostname

### `src/Comms.cpp` & `include/Comms.h`
**Purpose:** Network communication and web interface

**Key Responsibilities:**
- HTTP server on port 80
- Serve web UI HTML/CSS/JavaScript (stored in Flash using `F()` macro)
- REST API endpoints for timer control
- WebSocket connection management API
- Timer settings persistence (color thresholds, fonts, etc.)

**API Endpoints:**
- `GET /` - Web interface HTML
- `GET /api?action=<start|pause|reset>` - Timer control
- `GET /api?action=settings&duration=X&font=Y&spacing=Z` - Settings
- `POST /api/thresholds` - Update color thresholds
- `GET /api/thresholds` - Get current thresholds
- `GET /api/status` - Get timer state (paused/running)
- `GET /api/websocket/status` - WebSocket connection status
- `POST /api/websocket/connect` - Connect to WebSocket server
- `POST /api/websocket/disconnect` - Disconnect from WebSocket

**Critical Details:**
- Uses `client.print(F("..."))` to store strings in Flash memory (saves RAM)
- HTML is generated dynamically, split into chunks
- Three-column grid layout: Timer Controls | Color Thresholds & Font | WebSocket
- Responsive design: collapses to single column on narrow screens

### `src/Timer.cpp` & `include/Timer.h`
**Purpose:** Timer logic and state management

**Key Responsibilities:**
- Countdown/stopwatch timing calculations
- State machine: IDLE → RUNNING → PAUSED → EXPIRED
- Blink pattern control (different rates for paused/expired)
- Duration management
- Elapsed time tracking

**State Transitions:**
- `start()` - IDLE/PAUSED → RUNNING
- `pause()` - RUNNING → PAUSED
- `reset()` - ANY → IDLE
- Auto-transition: RUNNING → EXPIRED when time reaches zero

**Important Methods:**
- `setDuration({minutes, seconds, milliseconds})` - Set timer duration
- `getRemainingTime()` - Returns `TimeComponents` struct
- `isRunning()`, `isPaused()`, `isExpired()` - State queries
- `shouldBlink()` - Returns true during blink-off periods

### `src/TimerDisplay.cpp` & `include/TimerDisplay.h`
**Purpose:** Render timer on LED matrix

**Key Responsibilities:**
- Format time string (MM:SS, M:SS, SS.D based on duration)
- Center text on display
- Apply color based on thresholds
- Handle font switching (19 fonts available)
- Character spacing adjustment
- Blink animation (clear display during blink-off)

**Font System:**
- Font 0: Adafruit default 5x7 @ 2x scale
- Fonts 1-12: FreeFonts (Sans, Mono, Serif in 9pt/12pt, regular/bold)
- Fonts 13-15: Pixel fonts (Org_01, Picopixel, TomThumb)
- Fonts 16-18: Custom Aquire fonts (regular, bold, light)

**Color Threshold System:**
- Thresholds sorted by time (descending)
- First matching threshold applies
- Format: `{seconds, RGB565_color}`
- Default color when no threshold matches
- Example: Green > 2min, Yellow 1-2min, Red < 1min

### `src/WebSocketClient.cpp` & `include/WebSocketClient.h`
**Purpose:** WebSocket integration with FightTimer

**Key Responsibilities:**
- Connect to WebSocket server (typically Socket.IO)
- Parse JSON timer events
- Control Timer instance based on events
- Auto-reconnect on connection loss
- Event logging to Serial

**Supported Message Formats:**
```json
// Direct action
{"action": "start"}

// With duration
{"action": "reset", "minutes": 3, "seconds": 0}

// Socket.IO wrapper
{"timer_update": {"action": "start"}}

// Socket.IO array format
["timer_update", {"action": "start"}]
```

**Actions:**
- `start` - Start/resume timer
- `stop` - Pause timer
- `reset` - Reset timer with optional new duration
- `settings` - Update timer settings (logged, not applied to display yet)

**Critical Implementation Detail:**
- Uses **static callback pattern** because WebSocketsClient library requires C-style function pointer
- `_instance` static pointer allows static `webSocketEvent()` to call instance methods
- Only one WebSocketClient instance should exist

### `src/RGBMatrix.cpp` & `include/RGBMatrix.h`
**Purpose:** RGB matrix initialization wrapper

**Key Responsibilities:**
- Initialize Adafruit Protomatter library
- Configure matrix dimensions (64x32)
- Set bit depth and pin mapping
- Handle matrix orientation (180° rotation)

**Pin Configuration:**
- See Protomatter documentation for HUB75 wiring
- Default orientation: 180° to account for physical mounting

---

## Critical Design Patterns

### 1. Flash Memory Optimization
**Problem:** RP2040 has limited RAM (256KB)  
**Solution:** Use `F()` macro to store strings in Flash

```cpp
// BAD - uses RAM
client.print("<html><head>");

// GOOD - uses Flash
client.print(F("<html><head>"));
```

**Where Applied:**
- All HTML generation in `Comms.cpp`
- Long string constants
- Serial debug messages

### 2. Static Callback Pattern (WebSocketClient)
**Problem:** C++ library requires C-style function pointer for callback  
**Solution:** Static method + static instance pointer

```cpp
class WebSocketClient {
    static WebSocketClient* _instance;  // Points to singleton
    
    static void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
        if (_instance) {
            _instance->handleWebSocketEvent(type, payload, length);
        }
    }
    
    void handleWebSocketEvent(...) { /* actual implementation */ }
};
```

**Why:** Allows library to call static function, which then delegates to instance method

### 3. Time Format Auto-Switching
**Logic:** Display format changes based on remaining time
- `≥ 10 minutes`: `MM:SS` (e.g., "15:30")
- `< 10 minutes`: `M:SS` (e.g., "5:30")
- `< 1 minute`: `SS.D` (e.g., "45.7")

**Implementation:** `TimerDisplay::formatTime()`

### 4. Color Threshold Cascade
**Logic:** First matching threshold wins
- Thresholds checked in order (descending time)
- If remaining time ≤ threshold time, use that color
- If no match, use default color

**Storage Format:**
- Web API: `"120:FFFF00|60:FF0000"` (seconds:hexcolor)
- Internal: `std::vector<ColorThreshold>`

---

## Common Pitfalls and Solutions

### ❌ Pitfall 1: Using Wrong Timer API
```cpp
timer.setTime(180);  // ❌ This method doesn't exist!
```
**Solution:**
```cpp
timer.setDuration({3, 0, 0});  // ✅ minutes, seconds, milliseconds
timer.reset();                  // ✅ Apply the new duration
```

### ❌ Pitfall 2: Arduino String Methods
```cpp
if (str.isEmpty()) { }  // ❌ Arduino String doesn't have isEmpty()!
```
**Solution:**
```cpp
if (str.length() == 0) { }  // ✅ Use length() instead
```

### ❌ Pitfall 3: ArduinoJson v7 API Changes
```cpp
if (doc.containsKey("action")) { }  // ❌ Deprecated in v7!
```
**Solution:**
```cpp
if (doc["action"].is<const char*>()) { }  // ✅ Use .is<T>() checks
if (doc["action"].as<String>().length() > 0) { }  // ✅ Or check value
```

### ❌ Pitfall 4: WebSocket Library Incompatibility
```cpp
// ❌ Using gilmaimon/ArduinoWebsockets
// Problem: Missing WSDefaultTcpClient for non-ESP platforms
```
**Solution:**
```cpp
// ✅ Use links2004/WebSockets library
// Add to platformio.ini:
lib_deps = 
    links2004/WebSockets@^2.7.1
build_flags = 
    -D WEBSOCKETS_NETWORK_TYPE=NETWORK_W5100
```

### ❌ Pitfall 5: RAM Exhaustion
```cpp
String html = "<html><body>..."; // ❌ 10KB string in RAM!
html += "...more HTML...";
client.print(html);
```
**Solution:**
```cpp
client.print(F("<html><body>"));  // ✅ Stored in Flash
client.print(F("...more HTML..."));
```

### ❌ Pitfall 6: Incorrect HTML Structure
```cpp
// ❌ Missing closing tags or duplicates cause layout issues
client.print(F("</div></div>"));  // Which div are we closing?
```
**Solution:**
- Add comments: `client.print(F("</div>"));  // End column 2`
- Track nesting level carefully
- Test responsive layout at different widths

---

## API Reference

### REST API Quick Reference

#### Timer Control
```http
GET /api?action=start
GET /api?action=pause  
GET /api?action=reset
```

#### Settings
```http
GET /api?action=settings&duration=180&font=4&spacing=3
# duration: total seconds
# font: 0-18 (see Font System)
# spacing: -2 to 5 (character spacing in pixels)
```

#### Color Thresholds
```http
POST /api/thresholds
Content-Type: application/x-www-form-urlencoded

thresholds=120:FFFF00|60:FF0000&default=00FF00
# Format: seconds:RRGGBB (hex color)
# Separate multiple with |
```

```http
GET /api/thresholds
# Returns: {"thresholds":[{seconds:120,color:"#FFFF00"}],"defaultColor":"#00FF00"}
```

#### Status Check
```http
GET /api/status
# Returns: {"isPaused": true/false}
```

#### WebSocket Control
```http
GET /api/websocket/status
# Returns: {"connected": true/false, "status": "...", "url": "..."}

POST /api/websocket/connect
Content-Type: application/x-www-form-urlencoded

host=192.168.1.100&port=8765&path=/socket.io/

POST /api/websocket/disconnect
```

---

## WebSocket Integration

### FightTimer Integration
**Repository:** https://github.com/PongAlmighty/FightTimer  
**Protocol:** Socket.IO over WebSocket  
**Default Port:** 8765  
**Default Path:** `/socket.io/`

### Message Flow
```
FightTimer Server → WebSocket → Arena Timer → LED Display
     (JSON)                        (parse)      (render)
```

### Reconnection Logic
- Auto-reconnect on connection loss
- Reconnect attempt every few seconds
- Status displayed in web UI

### Testing WebSocket
```bash
# Use browser console or wscat to test
wscat -c ws://192.168.1.100:8765/socket.io/

# Send test message
{"action": "start"}
```

---

## Build System

### PlatformIO Configuration (`platformio.ini`)

**Platform:** Raspberry Pi RP2040 (custom git repo)
```ini
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = waveshare_rp2040_zero
framework = arduino
```

**Critical Build Flags:**
```ini
build_flags = 
    -D WEBSOCKETS_NETWORK_TYPE=NETWORK_W5100
```
**Why:** Tells WebSockets library to use W5100/W5500 Ethernet instead of WiFi

**Key Dependencies:**
- `Adafruit Protomatter` - RGB matrix driver
- `Ethernet` - W5500 support
- `EthernetBonjour` - mDNS/hostname resolution
- `WebSockets@2.7.1` - WebSocket client (must use links2004 version)
- `ArduinoJson@7.4.2` - JSON parsing

### Build Commands
```bash
# Clean build
pio run --target clean
pio run

# Upload firmware
pio run --target upload

# Monitor serial output
pio device monitor
```

### Build Artifacts
- `firmware.elf` - ELF binary
- `firmware.uf2` - UF2 bootloader format (drag-and-drop to RP2040)
- `firmware.bin` - Raw binary

---

## Development Workflow

### 1. Setting Up Development Environment
```bash
# Install PlatformIO
pip install platformio

# Clone repository
git clone <repo_url>
cd Arena-Timer-Firmware

# Build
pio run
```

### 2. Uploading Firmware
**Method A: Drag-and-Drop (Easy)**
1. Hold BOOT button while plugging in USB
2. RP2040 appears as USB drive
3. Drag `.pio/build/waveshare_rp2040_zero/firmware.uf2` to drive

**Method B: PlatformIO Upload**
```bash
pio run --target upload
```

### 3. Debugging
```bash
# Serial monitor (115200 baud)
pio device monitor

# Look for:
# - Ethernet initialization messages
# - IP address display confirmation
# - WebSocket connection status
# - API request logs
```

### 4. Testing Network
```bash
# Test mDNS
ping arenatimer.local

# Test direct IP
ping 10.0.0.177

# Test web interface
curl http://10.0.0.177/

# Test API
curl "http://10.0.0.177/api?action=start"
```

### 5. Common Development Tasks

#### Changing Default IP
**File:** `src/main.cpp`
```cpp
uint8_t static_ip[] = {10, 0, 0, 177};  // Change last octet
```

#### Adding New Font
1. Generate `.h` font file using Adafruit GFX Font Tool
2. Add to `include/CustomFonts/` directory
3. Include in `TimerDisplay.cpp`
4. Add case to `selectFont()` switch statement
5. Add option to font dropdown in `Comms.cpp` HTML

#### Modifying Web UI Layout
**File:** `src/Comms.cpp`
- CSS in `<style>` section (around line 270)
- HTML structure starts around line 328
- Three-column grid: `.grid-container` with `.grid-column` children
- Responsive breakpoint: `@media (max-width:1200px)`

#### Adding New API Endpoint
1. Parse URL path in `Comms::handleClient()`
2. Add new `else if` condition for your path
3. Parse query parameters or POST data
4. Call appropriate Timer/TimerDisplay methods
5. Send HTTP response using `sendHTTPResponse()` or `client.print()`

---

## Version History & Important Notes

### Current Version (2025-10-10)
- ✅ WebSocket client integration with FightTimer
- ✅ Three-column responsive grid UI
- ✅ Color threshold system with multiple levels
- ✅ 19 font options with character spacing control
- ✅ mDNS hostname support
- ✅ Comprehensive API endpoints

### Known Limitations
- WebSocket settings events logged but not applied to display (intentional)
- No persistence across reboots (network settings, thresholds reset)
- Single WebSocket client instance only
- No DHCP support (static IP only)
- Limited to 256KB RAM (careful with string storage)

### Future Enhancement Ideas
- EEPROM storage for settings persistence
- DHCP support with fallback to static IP
- OTA firmware updates
- Multiple simultaneous WebSocket clients
- Real-time clock integration for time-of-day display
- Sound effects via buzzer

---

## Quick Reference Cheat Sheet

### Important Pins
- W5500 CS: GPIO 1
- W5500 SCK: GPIO 2
- W5500 MOSI: GPIO 3
- W5500 MISO: GPIO 4
- HUB75: See `RGBMatrix.cpp`

### Default Settings
- IP: 10.0.0.177
- Hostname: arenatimer.local
- MAC: DE:AD:BE:EF:FE:ED
- HTTP Port: 80
- Serial Baud: 115200

### File Size Limits
- Flash: 2MB total, ~182KB used (~9%)
- RAM: 256KB total, ~10KB used (~4%)
- Keep RAM usage under 50% for stability

### Key Classes
- `Timer` - State machine and time calculations
- `TimerDisplay` - Rendering and formatting
- `Comms` - HTTP server and web UI
- `WebSocketClient` - FightTimer integration
- `RGBMatrix` - Display hardware abstraction

---

## Getting Help

### Debugging Checklist
1. ✓ Is Ethernet cable connected?
2. ✓ Is device powered on?
3. ✓ Check serial output for initialization messages
4. ✓ Verify IP address shown on LED matrix (green text, 5 seconds)
5. ✓ Can you ping the IP address?
6. ✓ Is mDNS service running? (Check serial output)
7. ✓ Try direct IP instead of hostname
8. ✓ Check build flags in `platformio.ini`
9. ✓ Verify correct libraries installed
10. ✓ Check for RAM exhaustion (reduce string usage)

### Common Error Messages
- `Ethernet initialization failed` - Check SPI wiring
- `No space in arena for` - RAM exhausted, use more `F()` macros
- `WebSocket connection failed` - Check host/port/path settings
- `Font not found` - Verify font header files included

---

## Conclusion

This guide should provide AI agents (and human developers) with a comprehensive understanding of the Arena Timer Firmware architecture, design patterns, and development workflow. When in doubt, refer to the actual source code and comments for the most up-to-date information.

**Key Takeaways:**
1. Use `F()` macro religiously to save RAM
2. Timer duration uses `setDuration()` + `reset()`, not `setTime()`
3. WebSocket library must be links2004 version with W5100 build flag
4. HTML structure has three columns, check closing tags carefully
5. ArduinoJson v7 uses `.is<T>()` instead of `containsKey()`

**When making changes:**
- Test at multiple screen widths (responsive design)
- Monitor RAM usage during compile
- Check serial output for errors
- Verify network connectivity still works
- Test all API endpoints after modifications

Good luck! 🤖⚔️
