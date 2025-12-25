# Hardware Pinout (VERIFIED WORKING)

**Device**: Arena Timer  
**Controller**: Silicognition RP2040-Shim  
**Network**: POE-FeatherWing (W5500)  
**Display**: HUB75 RGB LED Matrix (64x32)

## Matrix Pin Mapping
| Signal | GPIO | Board Label |
|--------|------|-------------|
| R1 | 16 | SDA |
| G1 | 17 | SCL |
| B1 | 20 | D9 |
| R2 | 6  | D4 |
| G2 | 19 | D6 |
| B2 | 25 | D25 |
| CLK | 22 | D13 |
| **LAT** | **1** | **RX** |
| **OE** | **0** | **TX** |
| A | 29 | A0 |
| B | 28 | A1 |
| C | 27 | A2 |
| D | 26 | A3 |

> **Note**: LAT and OE use GPIO 0/1 (TX/RX pins). USB Serial still works.

## Ethernet (SPI1)
| Signal | GPIO | Board Label |
|--------|------|-------------|
| SCK | 10 | SCK |
| MOSI | 11 | MOSI |
| MISO | 12 | MISO |
| CS | 21 | D10 |

## ⚠️ Reserved Pins
- **GPIO 21**: Ethernet CS - do not use for matrix
- **GPIO 24**: VBUS sensing - never use as output (will crash!)
- **GPIO 23**: Internal Neopixel
