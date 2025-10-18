# Arena Timer Firmware

A professional countdown timer system for competitive arenas, combat sports, and event timing. Built on the Waveshare RP2040-Zero with a vibrant 64x32 RGB LED matrix display, web-based control, and seamless integration with [FightTimer](https://github.com/PongAlmighty/FightTimer) by PongAlmighty.

## Features

### Display & Visual Customization
- **64x32 RGB LED Matrix** with full color control
- **Dynamic Color Thresholds** - automatically change colors as time decreases
- **Multiple Font Choices** - Sans, Serif, Monospace, Retro/Pixel styles
- **Adjustable Brightness** - 0-255 levels for any lighting condition
- **Character Spacing Control** - fine-tune text appearance
- **Display Rotation** - flip orientation 180° with one button

### Timer Controls
- **Countdown Mode** - configurable duration up to 60 minutes
- **Start/Pause/Reset** - full manual control
- **Sub-Second Precision** - displays tenths of seconds under 1 minute
- **Visual States** - blinking when paused, flashing when expired

### Web Interface
- **Responsive Three-Column Layout** - Timer controls, color settings, system status
- **Real-Time Console** - live event logging with timestamps
- **Live Updates** - automatic status refresh and button state management
- **Mobile Friendly** - works on phones, tablets, and desktops

### Network & Integration
- **DHCP Support** with static IP fallback (10.0.0.21)
- **mDNS Hostname** - access via `http://arenatimer.local`
- **FightTimer Integration** - Socket.IO connection for synchronized timing (credit: [PongAlmighty](https://github.com/PongAlmighty/))
- **RESTful API** - control timer programmatically

## Hardware

### Required Components
- **Waveshare RP2040-Zero** microcontroller
- **64x32 RGB LED Matrix Panel** (HUB75 interface, P5 pitch recommended)
- **W5500 Ethernet Module** (SPI interface)
- **5V Power Supply** (minimum 2A, 4A recommended for full brightness / more pixels)
  - I used a 5V 20W PoE splitter mounted to the back of the enclosure

### 3D Enclosure
3D model files for a custom LED matrix enclosure are available in the `3d-models/` directory:
- `Timer Assembly v23.step` - STEP format for CAD editing
- `Timer Assembly v23.f3z` - Fusion 360 archive format

Files are print-ready and designed for:
- Standard 64x32 P5 RGB matrix panels
- Electronics mounting (currently using custom protoboard assembly)
- PoE splitter bracket mount

## Quick Start

### 1. Flash Firmware
```bash
pio run --target upload
```

### 2. Network Connection
The timer will attempt DHCP, then fall back to `10.0.0.21` if unavailable. The assigned IP displays on the matrix for 5 seconds at startup.

### 3. Web Interface
Access the control panel at:
- `http://arenatimer.local` (mDNS)
- `http://[IP_ADDRESS]` (direct)

Use the web interface to:
- Start, pause, and reset the timer
- Set duration (minutes and seconds)
- Configure color thresholds for time-based alerts
- Adjust display settings (font, brightness, letter spacing)
- Flip display orientation
- Monitor system status and event logs

### 4. FightTimer Integration
To connect with FightTimer:
1. Ensure FightTimer is running on your network
2. In the **WebSocket Connection** section, enter:
   - **Host**: IP address of the computer running FightTimer
   - **Port**: `8765` (default Socket.IO port)
   - **Path**: `/socket.io/`
3. Click **Connect**

The timer will automatically sync with FightTimer's start/stop/reset commands and duration settings.

**What Gets Synchronized:**
- ✅ Timer start/stop/reset commands
- ✅ Duration changes
- ✅ Time remaining updates
- ✅ Expired/paused states

FightTimer sends `timer_update` events via Socket.IO that control the Arena Timer display.

**Alternative connection methods:**

Via code in `src/main.cpp`:
```cpp
void setup() {
    // ... existing setup code ...
    wsClient->connect("192.168.1.100", 8765, "/socket.io/");
}
```

Via API endpoint:
```bash
curl -X POST "http://arenatimer.local/api/websocket/connect" \
  -d "host=192.168.1.100&port=8765&path=/socket.io/"
```

## Configuration

### Network Settings
Edit `src/main.cpp` to change network defaults:
```cpp
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};  // MAC address
uint8_t static_ip[] = {10, 0, 0, 21};                   // Static IP fallback
const char* hostname = "arenatimer";                     // mDNS hostname
```

### Display Settings
Configure default display settings in `src/main.cpp`:
```cpp
// Set default font
timerDisplay.setFont(&FreeSansBold12pt7b);

// Set default duration (hours, minutes, seconds)
timerDisplay.getTimer().setDuration({0, 3, 0});  // 3 minutes

// Color thresholds (set via web interface or programmatically)
timerDisplay.addColorThreshold(120, 255, 255, 0);  // Yellow at 2 minutes
timerDisplay.addColorThreshold(60, 255, 0, 0);     // Red at 1 minute
```

### Available Fonts
- **Sans**: `FreeSans9pt7b`, `FreeSans12pt7b`, `FreeSansBold9pt7b`, `FreeSansBold12pt7b`
- **Serif**: `FreeSerif9pt7b`, `FreeSerif12pt7b`, `FreeSerifBold9pt7b`, `FreeSerifBold12pt7b`
- **Mono**: `FreeMono9pt7b`, `FreeMono12pt7b`, `FreeMonoBold9pt7b`, `FreeMonoBold12pt7b`
- **Retro**: `Org_01`, `Picopixel`, `TomThumb` (ultra-compact pixel fonts)
- **Custom**: `Aquire_BW0ox12pt7b`, `AquireBold_8Ma6012pt7b`, `AquireLight_YzE0o12pt7b`

### Pin Configuration
Default pins are defined in `include/RGBMatrix.h` for the Waveshare RP2040-Zero. Modify if using different pin connections.

### Performance Optimization
The firmware includes debug flags that can be disabled for optimal timing performance:

```cpp
// src/main.cpp
#define DEBUG_MAIN false

// src/WebServer.cpp
#define DEBUG_WEBSERVER false

// src/WebSocketClient.cpp
#define DEBUG_WEBSOCKET false
```

When all debug flags are `false`, Serial output is disabled, eliminating timing delays. This is recommended for production use.

## API Reference

The timer exposes a RESTful API for programmatic control:

### Timer Control
```bash
# Start timer
POST /api?action=start

# Pause timer
POST /api?action=pause

# Reset timer
POST /api?action=reset

# Flip display orientation
POST /api?action=flip
```

### Settings
```bash
# Update timer settings
POST /api?action=settings&duration=180&font=4&spacing=3&brightness=255

# Update color thresholds
POST /api/thresholds
Content-Type: application/x-www-form-urlencoded
thresholds=120:%23FFFF00|60:%23FF0000&default=%2300FF00
```

### Status Information
```bash
# Get timer status
GET /api/status

# Get network information
GET /api/network/status

# Get WebSocket connection status
GET /api/websocket/status
```

### WebSocket Connection
```bash
# Connect to FightTimer
POST /api/websocket/connect
Content-Type: application/x-www-form-urlencoded
host=192.168.1.100&port=8765&path=/socket.io/

# Disconnect
POST /api/websocket/disconnect
```

## Troubleshooting

### Display Issues
- **Blank display**: Check power supply (5V, minimum 2A recommended)
- **Corrupted display**: Verify HUB75 cable connections
- **Wrong colors**: Check RGB pin mappings in `src/RGBMatrix.cpp`

### Network Issues
- **Can't access web interface**: Check Ethernet cable, verify IP on display at startup
- **DHCP not working**: Timer falls back to static IP `10.0.0.21`
- **mDNS not resolving**: Try direct IP address instead

### FightTimer Connection
- **Won't connect**: Verify FightTimer is running and accessible at the specified host/port
- **Connects but no updates**: Check that FightTimer is sending `timer_update` Socket.IO events
- **Frequent disconnects**: Check network stability between devices

## Development

### Build Environment
- **Framework**: Arduino
- **Platform**: Raspberry Pi Pico (RP2040)
- **Tool**: PlatformIO

### Key Libraries
- **Adafruit Protomatter** - RGB matrix driver
- **Ethernet** - W5500 network interface
- **WebSockets** - Socket.IO client (links2004/arduinoWebSockets)
- **EthernetBonjour** - mDNS support

### Building from Source
```bash
# Clone repository
git clone https://github.com/EVAC-AZ/Arena-Timer-Firmware.git
cd Arena-Timer-Firmware

# Install dependencies
pio pkg install

# Build and upload
pio run --target upload

# Monitor serial output (optional)
pio device monitor
```

### Project Structure
```
arena-timer-firmware/
├── src/
│   ├── main.cpp              # Entry point and configuration
│   ├── Timer.cpp             # Core timer logic
│   ├── TimerDisplay.cpp      # LED matrix display control
│   ├── RGBMatrix.cpp         # Low-level matrix driver
│   ├── WebServer.cpp         # Web server and API
│   └── WebSocketClient.cpp   # Socket.IO client
├── include/
│   ├── Timer.h
│   ├── TimerDisplay.h
│   ├── RGBMatrix.h
│   ├── WebServer.h
│   ├── WebSocketClient.h
│   └── CustomFonts/          # Custom font definitions
├── 3d-models/                # Enclosure models
├── docs/                     # Documentation
├── platformio.ini            # Build configuration
└── README.md
```

### Future Plans
A standalone Arduino/PlatformIO library is planned to simplify integration of this timer system into other projects. The goal is to separate the timing/web UI/FightTimer integration code from the hardware-specific code, allowing use with different display hardware and microcontrollers (ESP32, ESP8266, etc.).

## Credits

- **FightTimer Integration**: [PongAlmighty/FightTimer](https://github.com/PongAlmighty/FightTimer) - Synchronized timing system for combat sports
- **RGB Matrix Control**: Adafruit Protomatter library
- **WebSocket Library**: Arduino WebSockets by links2004
- **Enclosure Design**: EVAC-AZ

## License

This project is open source. See `LICENSE` for details.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes with clear commits
4. Submit a pull request

For bug reports and feature requests, open an issue on GitHub.

---

**Built for arena timing excellence** 🏆