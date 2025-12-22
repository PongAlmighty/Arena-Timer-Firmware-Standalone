# Hardware Pinout & Wiring Guide

**Device**: Arena Timer (V2 Firmware)
**Controller**: Raspberry Pi Pico (RP2040)
**Interface Board**: Silicognition RP2040-Shim
**Network**: Silicognition POE-FeatherWing (or compatible Ethernet FeatherWing)
**Display**: Hub75 RGB LED Matrix (Waveshare or similar) via **Custom Adapter Board**

## Hardware Context

This project uses a **Silicognition RP2040-Shim** which maps the Pico's GPIOs to a standard Feather form factor. We are using a **POE FeatherWing** for Ethernet, which requires the standard SPI pins (SCK, MOSI, MISO) plus a Chip Select (CS).

Because the POE Wing uses the default SPI pins (D24, D25, SCK, MOSI, MISO), we cannot use the standard "Adafruit RGB Matrix FeatherWing" pinout, as it conflicts with SPI.

**Solution**: A custom adapter board was built to map the Matrix Hub75 pins to available "free" pins on the Feather header.

---

## 16-Pin IDC Connector Pinout (Custom Adapter)

**Orientation**: Looking at the Male Header pins on the prototype board.
**Pin 1**: Indicated by the **Red Stripe** on the ribbon cable.

```
       [Red Stripe / Pin 1 Side]
        +-------------------+
  R1    |  1 (SDA)  2 (SCL) | G1
  B1    |  3 (D11)  4 (GND) | GND
  R2    |  5 (D4)   6 (D10) | G2
  B2    |  7 (D25)  8 (GND) | GND
   A    |  9 (A0)  10 (A1)  | B
   C    | 11 (A2)  12 (A3)  | D
 CLK    | 13 (D13) 14 (Tx)  | LAT
  OE    | 15 (Rx)  16 (GND) | GND
        +-------------------+
```

---

## Pin Mapping Table

| Hub75 Signal | Feather Pin Name | RP2040 GPIO | Function/Notes |
| :--- | :--- | :--- | :--- |
| **R1** | **SDA** | 2 | Top-red Data |
| **G1** | **SCL** | 3 | Top-green Data |
| **B1** | **D11** | 11 | Top-blue Data |
| **R2** | **D4** | 6 | Bottom-red Data |
| **G2** | **D10** | 21 | Bottom-green Data |
| **B2** | **D25** | 25 | Bottom-blue Data |
| **CLK** | **D13** | 22 | Clock |
| **LAT** | **TX** | 0 | Latch |
| **OE** | **RX** | 1 | Output Enable |
| **A** | **A0** | 29 | Address A |
| **B** | **A1** | 28 | Address B |
| **C** | **A2** | 27 | Address C |
| **D** | **A3** | 26 | Address D |
| **CS** (Ethernet) | **D24** | 24 | **Hardwired** (Cut trace on Shim/Wing if needed, re-routed to D24) |

### Notes
- **Power**: Matrix is powered directly from 5V PSU (Red/Black spade cables). 
- **Ground**: **CRITICAL** - The Matrix Ground must be connected to the Pico/Shim Ground.
- **Ethernet**: Uses SPI on SCK(18), MOSI(19), MISO(16/20). **CS** is on **D24** (GPIO 24).
