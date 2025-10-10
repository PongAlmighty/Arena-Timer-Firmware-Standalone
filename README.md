# Arena Timer Firmware

Professional countdown timer for combat robotics arenas, featuring a large 64x32 RGB LED matrix display with multiple control methods including livestream integration.

## Overview

This firmware transforms a Waveshare RP2040-Zero and RGB matrix into a network-controlled arena timer. Built specifically for combat robotics events, it provides precise countdown timing with multiple control interfaces and livestream synchronization capabilities.

## Hardware Requirements

- **Microcontroller**: Waveshare RP2040-Zero
- **Display**: Waveshare RGB-Matrix-P5-64x32 (HUB75 interface)
- **Network**: Wiznet W5500 Ethernet module
- **Power**: 5V power supply (adequate current for RGB matrix)

## Key Features

- **Large, Bright Display** - 64x32 RGB matrix with customizable colors and fonts
- **Multiple Control Methods** - Web UI, FightTimer integration, or direct WebSocket
- **Network Connectivity** - Static IP with mDNS hostname support
- **Precision Timing** - Millisecond accuracy with visual state indicators
- **Livestream Integration** - Seamless synchronization with broadcast software

## Quick Start

1. **Hardware Setup**: Connect W5500 Ethernet module via SPI and RGB matrix via HUB75
2. **Upload Firmware**: Use PlatformIO to flash the RP2040-Zero
3. **Network Configuration**: Device uses static IP `10.0.0.21` by default
4. **Access Interface**: Navigate to `http://arenatimer.local` or the device IP
5. **FightTimer Integration**: Use WebSocket connection for instant livestream synchronization

## Control Methods

### 1. Web Interface (Built-in)

**Access**: `http://arenatimer.local` or device IP address

The responsive web interface provides:
- **Timer Controls**: Start, pause, reset with precise duration setting
- **Display Customization**: Color picker, font selection, and text scaling
- **Live Status**: Real-time timer state and connection monitoring
- **WebSocket Configuration**: Direct connection to external servers

### 2. FightTimer Integration (Recommended for Livestreams)

**Credit**: Integration with [FightTimer](https://github.com/PongAlmighty/FightTimer) by [PongAlmighty](https://github.com/PongAlmighty) - an excellent combat robotics livestream timer system.

#### **Direct WebSocket Connection (Preferred)**

**✅ Completely Self-Contained** - No Python dependencies or external scripts needed!

1. **Run FightTimer** normally on your computer
2. **Open Arena Timer web interface**: `http://arenatimer.local`
3. **Configure WebSocket connection**:
   - **Host**: IP address where FightTimer is running (e.g., `10.0.0.1`)
   - **Port**: `8765` (FightTimer's default WebSocket port)
   - **Path**: `/socket.io/`
4. **Click Connect** - Timer automatically syncs with FightTimer!
5. **Control from FightTimer's interface** - Physical timer responds with the FightTimer Control Panel

**Features**:
- **Zero Dependencies**: Built into the timer firmware
- **Tight Synchronization**: Direct Socket.IO connection to FightTimer
- **Automatic Reconnection**: Handles network interruptions gracefully
- **Real-time Status**: Connection status visible in web interface

#### **Python Controller Method (Alternative)**

For users who prefer a bridged approach or need custom integration:

1. Install dependencies: `pip install -r requirements.txt`
2. Configure IPs in `arena_timer_controller.py`
3. Run FightTimer normally
4. Run: `python arena_timer_controller.py`
5. Control from FightTimer's web interface

**Features**:
- **Zero FightTimer Modifications**: Works with standard FightTimer installation
- **HTTP API Control**: Uses Arena Timer's REST API
- **Robust Connection**: Automatic reconnection and error handling
- **Debug Control**: Configurable logging for performance optimization

### 3. WebSocket Bridge (Legacy/Advanced)

**Credit**: For advanced users needing custom WebSocket bridging with [FightTimer](https://github.com/PongAlmighty/FightTimer) by [PongAlmighty](https://github.com/PongAlmighty).

**Bridge Method**:
1. Install dependencies: `pip install -r requirements.txt`
2. Run FightTimer normally
3. Run: `python websocket_bridge.py` 
4. Connect timer to bridge via web interface:
   - **Host**: `localhost` or IP where bridge is running
   - **Port**: `8766` (bridge WebSocket port)
   - **Path**: `/ws`
5. Timer receives events through the bridge protocol

**Features**:
- **Protocol Translation**: Converts Socket.IO to plain WebSocket
- **Custom Integration**: Allows modification of message formats
- **Legacy Compatibility**: For testing or custom Socket.IO scenarios

**Use Cases**:
- Multiple timer synchronization across different locations
- Custom protocol translation or message filtering
- Development and testing of WebSocket integrations
- When direct Socket.IO connection needs customization

**Note**: The direct WebSocket connection (Method 2A) is now preferred for most users as it eliminates the need for this bridge.

## Timer Display

**Automatic Format Switching**:
- `15:30` - Minutes and seconds (≥10 min)
- `5:30` - Single digit minutes (<10 min)  
- `45.7` - Seconds with decimal precision (<1 min)

**Visual States**:
- **Idle**: Steady display showing set duration
- **Running**: Steady countdown display
- **Paused**: Slow blink (1 second intervals)
- **Expired**: Rapid flash (500ms intervals)

## Network Configuration

**Default Settings**:
- IP Address: `10.0.0.21`
- Hostname: `arenatimer.local`
- MAC: `DE:AD:BE:EF:FE:ED`

**Customization**: Edit network settings in `src/main.cpp`

**Network Tips**:
- Use static IP subnet (e.g., `10.0.0.x`)
- No DHCP server required
- Consider VLAN isolation for arena networks

## API Reference

**REST Endpoints**:
- `GET /` - Web interface
- `GET /api?action=start&minutes=3&seconds=0` - Timer control
- `POST /api/websocket/connect` - WebSocket connection management

**WebSocket Protocol**:
```json
{"action": "start", "minutes": 3, "seconds": 0}
{"action": "stop"}
{"action": "reset", "minutes": 3, "seconds": 0}
```

## Performance Optimization

**Debug Flags** (for reduced latency):
- **Python Controller**: Set `DEBUG_OUTPUT = False`
- **Arduino Main**: Set `#define DEBUG_MAIN false`
- **Arduino WebSocket**: Set `#define DEBUG_WEBSOCKET false`
- **WebSocket Bridge**: Set `DEBUG_FORWARDING = False`

Disabling debug output significantly improves response times and timing consistency.

## Troubleshooting

**Connection Issues**:
1. Verify IP address displayed on matrix at startup
2. Ensure same subnet (`10.0.0.x`)
3. Test with `ping 10.0.0.21`
4. Check serial monitor for initialization status

**Display Issues**:
1. Verify RGB matrix power supply adequacy (5V @4A recommended)
2. Check HUB75 cable connections
3. Try different font sizes and character spacings if text doesn't fit

## Development

**Project Structure**:
```
src/
├── main.cpp              # Main application
├── Timer.cpp             # Timing logic
├── TimerDisplay.cpp      # Display rendering
├── Comms.cpp            # Web server & API
└── WebSocketClient.cpp   # WebSocket integration

include/                  # Header files
arena_timer_controller.py # FightTimer integration
arduino_websocket_bridge.py # WebSocket bridge
requirements.txt         # Python dependencies
```

**Build System**: PlatformIO with RP2040 platform

## Credits

**FightTimer Integration**: Special thanks to [PongAlmighty](https://github.com/PongAlmighty) for creating [FightTimer](https://github.com/PongAlmighty/FightTimer), the excellent combat robotics livestream timer system that inspired this hardware integration.

**Libraries**:
- [Adafruit Protomatter](https://github.com/adafruit/Adafruit_Protomatter) - RGB matrix driver
- [Arduino Ethernet](https://github.com/arduino-libraries/Ethernet) - W5500 support
- [WebSockets](https://github.com/Links2004/arduinoWebSockets) - WebSocket client
- [ArduinoJson](https://arduinojson.org/) - JSON parsing

## License

See [LICENSE](LICENSE) file for details.

---

**Built for Combat Robotics** 🤖⚔️ | **Livestream Ready** 📡 | **Arena Tested** ✅
