---
description: RP2040-Shim Ethernet Hardware Setup and Recovery
---

# RP2040-Shim Ethernet Hardware Configuration

This workflow documents the stable hardware and software configuration discovered for the Silicognition RP2040-Shim with the PoE FeatherWing.

## 1. Hardware Pinout (Critical)

| Signal | GPIO (Raw) | Physical Label | Note |
|--------|------------|----------------|------|
| **SCK**  | 10         | SCK            | Part of SPI1 Block |
| **MOSI** | 11         | MO (MOSI)      | Part of SPI1 Block |
| **MISO** | 12         | MI (MISO)      | Part of SPI1 Block |
| **CS**   | 21         | D10            | **MUST BE 21**. |

### ⚠️ CRITICAL WARNING: GPIO 24
**NEVER** use GPIO 24 for Ethernet Chip Select (CS) or any high-speed SPI signal. On RP2040 designs (including the Shim), GPIO 24 is hardwired to VBUS (USB Power) sensing. Sending SPI data to this pin causes hardware-level electrical contention, leading to:
- Periodic hardware lockups.
- USB Serial disconnection from the host (Mac/PC).
- Immediate chip crashes during `Ethernet.init()`.

## 2. Software Requirements

### Library
Use **`khoih-prog/Ethernet_Generic`**.
The standard `arduino-libraries/Ethernet` often defaults to `SPI0` and lacks robust support for remapping to `SPI1` on the RP2040 without significant hacking. `Ethernet_Generic` allows passing the `SPI1` instance directly in the `begin()` call.

### PlatformIO Configuration (`platformio.ini`)
```ini
[env:pico]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = pico
framework = arduino
lib_deps = 
    khoih-prog/Ethernet_Generic@^2.8.1
```

## 3. Initialization Pattern

When initializing the Ethernet controller, use this exact sequence to ensure the RP2040 uses the correct SPI bus:

```cpp
#include <SPI.h>
#include <Ethernet_Generic.h>

void setup() {
  // 1. Manually configure SPI1 (Hardware pins 10, 11, 12)
  SPI1.setSCK(10);
  SPI1.setTX(11);
  SPI1.setRX(12);
  SPI1.begin();

  // 2. Initialize with CS Pin 21
  Ethernet.init(21);

  // 3. Start Ethernet passing the SPI1 object
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  if (Ethernet.begin(mac, &SPI1) == 0) {
    // Handle failure
  }
}
```

## 4. Troubleshooting
- **No Hardware Found**: Ensure the solder trace for the CS pin is repaired and not bridged to neighbors. GPIO 21 should have a clean path to the PoE FeatherWing's CS pad.
- **DHCP Fails**: Ensure the board is getting sufficient power. While USB can work, true PoE power (via the splitter) is more stable for the W5500's high current draw during link negotiation.
- **Mac Disconnects**: If the USB port vanishes, check for a short on GPIO 24 or an attempt to use it in code.
