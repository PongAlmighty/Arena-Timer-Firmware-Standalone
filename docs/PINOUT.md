# Hardware Pinout (FINAL - Verified)

**Device**: Arena Timer  
**Controller**: Silicognition RP2040-Shim  
**Network**: POE-FeatherWing (W5500)  
**Display**: HUB75 RGB LED Matrix

## 16-Pin Connector Wiring
```
       [Red Stripe / Pin 1]
        +-------------------+
  R1    |  1 (SDA)  2 (SCL) | G1
  B1    |  3 (D9)   4 (GND) | GND
  R2    |  5 (D4)   6 (D10) | G2
  B2    |  7 (D25)  8 (GND) | GND
   A    |  9 (A0)  10 (A1)  | B
   C    | 11 (A2)  12 (A3)  | D
 CLK    | 13 (D13) 14 (D12) | LAT
  OE    | 15 (D11) 16 (GND) | GND
        +-------------------+
```

## Pin Mapping Table
| Signal | Feather Label | GPIO | Notes |
|--------|---------------|------|-------|
| **R1** | **SDA** | **16** | |
| **G1** | **SCL** | **17** | |
| **B1** | **D9** | **20** | |
| **R2** | **D4** | **6** | |
| **G2** | **D10** | **21** | |
| **B2** | **D25** | **25** | |
| **CLK** | **D13** | **22** | |
| **LAT** | **D12** | **14** | Moved to avoid Serial conflict |
| **OE** | **D11** | **15** | Moved to avoid Serial conflict |
| **A** | **A0** | **29** | |
| **B** | **A1** | **28** | |
| **C** | **A2** | **27** | |
| **D** | **A3** | **26** | |
| **CS** | **D24** | **24** | Ethernet Chip Select |

## Reserved Pins (Do NOT Use)
- **TX (GPIO 0)** / **RX (GPIO 1)**: Serial Debug
- **SCK (GPIO 10)** / **MOSI (GPIO 11)** / **MISO (GPIO 12)**: Ethernet SPI
