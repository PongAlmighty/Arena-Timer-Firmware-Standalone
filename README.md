# Arena-Timer-Firmware

Embedded firmware for a custom arena timer, built on the Waveshare RGB-Matrix-P5-64x32 driven by a Waveshare RP2040-Zero and controlled through an ethernet interface with the W5500.

## Features

✨ **Large LED Matrix Display** - Crisp, centered countdown timer on 64x32 RGB matrix  
🌐 **Web Interface** - Clean, modern control interface accessible via Ethernet  
🔌 **Static IP + mDNS** - Reliable connectivity with hostname support  
🎨 **Customizable** - Control colors, fonts, text size, and duration from the web UI  
⏱️ **Multiple Modes** - Countdown timer or stopwatch modes  
💡 **Visual Feedback** - Different blink patterns for idle, paused, and expired states  

## Hardware Requirements

- **Microcontroller**: Waveshare RP2040-Zero
- **Display**: Waveshare RGB-Matrix-P5-64x32 (HUB75 interface)
- **Network**: Wiznet W5500 Ethernet module
- **Power**: 5V power supply for RGB matrix

## Getting Started

### 1. Upload Firmware

1. Install [PlatformIO](https://platformio.org/)
2. Clone this repository
3. Open in PlatformIO
4. Upload to your RP2040-Zero

### 2. Connect to Network

**Physical Connections:**
- Connect W5500 to RP2040-Zero via SPI:
  - CS (Chip Select) → GPIO 1
  - SCK (Serial Clock) → GPIO 2
  - MOSI (Master Out Slave In) → GPIO 3
  - MISO (Master In Slave Out) → GPIO 4
- Connect Ethernet cable from W5500 to your network switch
- Power on the device

**Network Configuration:**
The timer uses a **static IP address: 10.0.0.177** by default. On first boot, the IP address is displayed on the LED matrix in green for 5 seconds.

**To change the IP address**, edit the `static_ip` array in `main.cpp`:
```cpp
uint8_t static_ip[] = {10, 0, 0, 177};  // Change to your desired IP
```

### 3. Access Web Interface

You can access the timer control interface in two ways:

#### Option A: Use Hostname (Recommended)
Simply open your browser and navigate to:
```
http://arenatimer.local
```

> **Note**: mDNS (hostname resolution) works automatically on:
> - **macOS** - Built-in support
> - **Linux** - Install `avahi-daemon`
> - **Windows** - Install [Bonjour Print Services](https://support.apple.com/kb/DL999)

#### Option B: Use IP Address
If hostname doesn't work, check the Serial Monitor or LED matrix display for the IP address, then navigate to:
```
http://192.168.x.x
```

## Web Interface Usage

The web interface provides complete control over the timer:

### Duration Settings
- **Minutes**: Set 0-99 minutes
- **Seconds**: Set 0-59 seconds  
- **Milliseconds**: Set 0-999 milliseconds (in 100ms increments)

### Display Settings
- **Color Picker**: Choose any RGB color for the timer display
- **Font Selection**: 5 font options from small (9pt) to extra large (24pt)
- **Text Size**: 1x, 2x, or 3x scaling multiplier

### Controls
- **Start** - Begin countdown (or resume if paused)
- **Pause** - Pause the timer (can be resumed)
- **Reset** - Stop and reset to initial duration

### Timer States
- **Idle** - Timer reset, showing initial duration (steady display)
- **Running** - Timer actively counting down (steady display)
- **Paused** - Timer stopped mid-countdown (slow blink, 1 second intervals)
- **Expired** - Timer reached zero (rapid flash, 500ms intervals)

## Display Formats

The timer automatically switches formats based on remaining time:

- **≥10 minutes**: `MM:SS` format (e.g., `15:30`)
- **<10 minutes**: `M:SS` format (e.g., `5:30`)
- **<1 minute**: `SS.D` format (e.g., `45.7` = 45.7 seconds)

## Network Configuration

### Default Settings
- **MAC Address**: `DE:AD:BE:EF:FE:ED`
- **Static IP**: `10.0.0.177`
- **Hostname**: `arenatimer` (via mDNS)

### Customization
To change network settings, edit `main.cpp`:

```cpp
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};     // MAC address
uint8_t static_ip[] = {10, 0, 0, 177};                     // Static IP
const char* hostname = "arenatimer";                       // mDNS hostname
```

### Network Setup Tips
1. **Ensure all devices use the same subnet** (e.g., `10.0.0.x`)
2. **Set your PC's Ethernet adapter** to a static IP like `10.0.0.1` with subnet mask `255.255.255.0`
3. **Configure your switch** (if managed) to use a static IP in the same subnet (e.g., `10.0.0.10`)
4. **No DHCP server is required** - all devices use static IPs

### VLAN Setup (Advanced)
For arena setups with multiple devices, consider using PortVLAN on your managed switch to isolate control systems from public/robot networks.

## Troubleshooting

### Can't Access Web Interface

1. **Check Serial Monitor** - Verify Ethernet initialization succeeded
2. **Check LED Matrix** - IP address is displayed in green for 5 seconds on startup
3. **Verify Same Subnet** - Ensure timer (`10.0.0.177`) and PC are on same subnet (e.g., `10.0.0.x`)
4. **Check PC IP** - Set your PC's Ethernet adapter to static IP like `10.0.0.1` with mask `255.255.255.0`
5. **Try IP Address** - If hostname doesn't work, use the IP directly: `http://10.0.0.177`
6. **Check Cables** - Verify Ethernet cable is properly connected
7. **Ping Test** - Try `ping 10.0.0.177` to verify connectivity

### Ethernet Not Initializing

1. **Check Wiring** - Verify W5500 SPI connections (CS, SCK, MOSI, MISO)
2. **Check Power** - W5500 requires stable 3.3V power
3. **Serial Debug** - Monitor serial output for error messages

### Display Issues

1. **Check Orientation** - Display orientation set to 180° by default
2. **Check Power** - RGB matrix requires adequate 5V power supply
3. **Font Size** - Try smaller font/text size if text doesn't fit

## Development

### Project Structure
```
include/
  ├── Comms.h          - Ethernet and web server
  ├── RGBMatrix.h      - RGB matrix interface
  ├── Timer.h          - Timer logic and state management
  └── TimerDisplay.h   - Timer rendering and display
src/
  ├── main.cpp         - Main application
  ├── Comms.cpp        - Web server and API implementation
  ├── RGBMatrix.cpp    - Matrix initialization
  ├── Timer.cpp        - Timer calculations
  └── TimerDisplay.cpp - Display rendering and formatting
```

### API Endpoints

The web interface uses a simple REST API:

**GET** `/` - Serve web interface HTML  
**GET** `/api?action=<start|stop|reset>&minutes=<M>&seconds=<S>&milliseconds=<MS>&color=<HEX>&font=<0-4>&size=<1-3>`

Example:
```
http://arenatimer.local/api?action=start&minutes=5&seconds=30&color=FF0000&font=2&size=2
```

## License

See [LICENSE](LICENSE) file for details.

## Credits

Built using:
- [Adafruit Protomatter](https://github.com/adafruit/Adafruit_Protomatter) - RGB matrix driver
- [Arduino Ethernet Library](https://github.com/arduino-libraries/Ethernet) - W5500 support
- [EthernetBonjour](https://github.com/TrippyLighting/EthernetBonjour) - mDNS/hostname resolution

---

**Made for combat robotics arenas** 🤖⚔️
