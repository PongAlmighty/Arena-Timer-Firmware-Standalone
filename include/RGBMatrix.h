/**
 * Header for interfacing with the WaveShare RGB Matrix Panel
 */

#pragma once

#include <Adafruit_Protomatter.h>

namespace RGBMatrix {
void init();
void demo();
void setOrientation(int orientation);
void clear();
void show();
void minimalTest();

// Get reference to the matrix for advanced usage
Adafruit_Protomatter &getMatrix();
} // namespace RGBMatrix