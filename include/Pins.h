#pragma once

/**
 * Pinout definitions for the Arena Timer hardware
 */

namespace Pins
{
    // Wiznet W5500 Ethernet Module pin connections
    struct W5500
    {
        static constexpr int CS = 1;   // Chip Select
        static constexpr int SCK = 2;  // Serial Clock
        static constexpr int MOSI = 3; // Master Out Slave In
        static constexpr int MISO = 4; // Master In Slave Out
    };

    // Waveshare RGB-Matrix-P5-64x32 pin connections
    struct Display
    {
        static constexpr int A = 7;
        static constexpr int B = 10;
        static constexpr int C = 8;
        static constexpr int D = 11;
        static constexpr int E = 9;
        static constexpr int R1 = 14;
        static constexpr int G1 = 28;
        static constexpr int B1 = 15;
        static constexpr int R2 = 5;
        static constexpr int G2 = 29;
        static constexpr int B2 = 6;
        static constexpr int CLK = 26;
        static constexpr int LAT = 12;
        static constexpr int OE = 27;
    };
}