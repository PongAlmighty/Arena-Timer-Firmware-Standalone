# PoE-FeatherWing Reference Guide

Source: [Silicognition PoE-FeatherWing Product Page](https://silicognition.com/Products/poe-featherwing/)

## Overview
The PoE-FeatherWing is a drop-in replacement for the Adafruit Ethernet FeatherWing, adding Power over Ethernet (PoE) capability. It uses the **WIZnet W5500** Ethernet controller.

## Key Features
- **PoE (Isolated)**: IEEE 802.3at Class 1, Mode A and Mode B.
- **Output Power**: Up to 4 W available for the Feather and peripherals.
- **Unique MAC Address**: Built-in Microchip 24AA02E48 provides a globally unique MAC address via I2C.
- **Compatibility**: 
  - Standard Arduino Ethernet driver.
  - MicroPython/CircuitPython WIZNET5K driver.
  - Drop-in replacement for Adafruit Ethernet FeatherWing.

## Hardware Details & Jumpers
- **Solder Jumpers**:
  - **IRQ**: Allows connecting the W5500 IRQ signal to a Feather pin (default is disconnected).
  - **I2C Addr**: Configure the address of the MAC address chip (24AA02E48). Default I2C address is **0x50**.
- **Solder Pads**:
  - Available for additional ground or power signaling on the bottom of the board.

## Software Integration (Arduino)
The board works with the standard `Ethernet` library, but for RP2040-Shim specifically, initialization requires directing the library to the correct SPI bus and CS pin.

### Reading the Unique MAC address (I2C)
The MAC address is stored in the 24AA02E48 chip at I2C address **0x50**. 
Registers for MAC: `0xFA` through `0xFF`.

```cpp
#include <Wire.h>
byte mac[6];
void readMac() {
  Wire.beginTransmission(0x50);
  Wire.write(0xFA);
  Wire.endTransmission();
  Wire.requestFrom(0x50, 6);
  for (int i=0; i<6; i++) mac[i] = Wire.read();
}
```

## Physical Footprint
- Identical to Adafruit Feather footprint.
- Optimized for clearance around the RJ45 jack when used with the RP2040-Shim.
