/**
 * Source code for interfacing with the WaveShare RGB Matrix Panel
 */

#include <Adafruit_Protomatter.h>
#include <Arduino.h>
#include <RGBMatrix.h>

namespace RGBMatrix {
// Pin definitions for Silicognition PoE-FeatherWing on RP2040-Shim
// CORRECTED: Using actual Shim GPIO numbers matching user's current wiring
const int PIN_SDA = 16; // R1 -> SDA (IO16) - User confirmed wiring
const int PIN_SCL = 17; // G1 -> SCL (IO17) - User confirmed wiring
const int PIN_D9 = 20;  // B1 -> D9  (IO20)

const int PIN_D4 = 6;   // R2 -> D4  (IO6)
const int PIN_D6 = 19;  // G2 -> D6 (IO19) - FIXED: GPIO21 is Ethernet CS!
const int PIN_D25 = 25; // B2 -> D25 (IO25)

const int PIN_D13 = 22; // CLK -> D13 (IO22)
const int PIN_RX = 1;   // LAT -> RX (IO1) - USB Serial still works
const int PIN_TX = 0;   // OE  -> TX (IO0) - USB Serial still works

const int PIN_A0 = 29; // A   -> A0 (IO29)
const int PIN_A1 = 28; // B   -> A1 (IO28)
const int PIN_A2 = 27; // C   -> A2 (IO27)
const int PIN_A3 = 26; // D   -> A3 (IO26)

// Note: Ethernet CS (GPIO21) is handled by WebServer module

static constexpr int A = PIN_A0;
static constexpr int B = PIN_A1;
static constexpr int C = PIN_A2;
static constexpr int D = PIN_A3;

static constexpr int R1 = PIN_SDA;
static constexpr int G1 = PIN_SCL;
static constexpr int B1 = PIN_D9;

static constexpr int R2 = PIN_D4;
static constexpr int G2 = PIN_D6;
static constexpr int B2 = PIN_D25;

static constexpr int CLK = PIN_D13;
static constexpr int LAT = PIN_RX;
static constexpr int OE = PIN_TX;

uint8_t rgbPins[] = {R1, G1, B1, R2, G2, B2};
uint8_t addrPins[] = {A, B, C, D};

Adafruit_Protomatter matrix(64,       // matrix chain width
                            4,        // bitDepth (4 = 16 shades = 4096 colors)
                            1,        // rgbCount
                            rgbPins,  // rgbList
                            4,        // addrCount
                            addrPins, // addrList
                            CLK,      // clockPin
                            LAT,      // latchPin
                            OE,       // oePin
                            true      // doubleBuffer
);

void init() {
  // Initialization code for the RGB matrix
  // Initialize matrix...
  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if (status != PROTOMATTER_OK) {
    Serial.println("ERROR: Matrix initialization failed!");
    return;
  }
}

void demo() {
  // Demo code for the RGB matrix
  // Make four color bars (red, green, blue, white) with brightness ramp:
  for (int x = 0; x < matrix.width(); x++) {
    uint8_t level = x * 256 / matrix.width(); // 0-255 brightness
    matrix.drawPixel(x, matrix.height() - 4, matrix.color565(level, 0, 0));
    matrix.drawPixel(x, matrix.height() - 3, matrix.color565(0, level, 0));
    matrix.drawPixel(x, matrix.height() - 2, matrix.color565(0, 0, level));
    matrix.drawPixel(x, matrix.height() - 1,
                     matrix.color565(level, level, level));
  }

  // Simple shapes and text, showing GFX library calls:
  matrix.drawCircle(12, 10, 9, matrix.color565(255, 0, 0));   // Red
  matrix.drawRect(14, 6, 17, 17, matrix.color565(0, 255, 0)); // Green
  matrix.drawTriangle(32, 9, 41, 27, 23, 27,
                      matrix.color565(0, 0, 255)); // Blue

  const char *text = "3:00";
  int16_t x1, y1;
  uint16_t w, h;
  matrix.getTextBounds((char *)text, 0, 0, &x1, &y1, &w, &h);

  // Compute centered position
  int16_t cx = (matrix.width() - w) / 2;
  int16_t cy = (matrix.height() - h) / 2;

  matrix.setCursor(cx, cy); // top-left corner (in rotated space)
  matrix.println(text);     // Default text color is white

  // AFTER DRAWING, A show() CALL IS REQUIRED TO UPDATE THE MATRIX!

  matrix.show(); // Copy data to matrix buffers
}

// simple wrapper function to set display rotation based on degrees
void setOrientation(int orientation) {
  switch (orientation) {
  case 0:
    matrix.setRotation(0);
    break;
  case 90:
    matrix.setRotation(1);
    break;
  case 180:
    matrix.setRotation(2);
    break;
  case 270:
    matrix.setRotation(3);
    break;
  }
}

void clear() { matrix.fillScreen(0); }

void show() { matrix.show(); }

Adafruit_Protomatter &getMatrix() { return matrix; }

void minimalTest() {
  static uint8_t color_index = 0;
  static unsigned long last_change = 0;

  // Change color every 500ms
  if (millis() - last_change > 500) {
    last_change = millis();
    color_index = (color_index + 1) % 4;
  }

  // Fill entire screen with bright colors
  uint16_t color;
  switch (color_index) {
  case 0:
    color = matrix.color565(255, 0, 0);
    break; // Bright RED
  case 1:
    color = matrix.color565(0, 255, 0);
    break; // Bright GREEN
  case 2:
    color = matrix.color565(0, 0, 255);
    break; // Bright BLUE
  case 3:
    color = matrix.color565(255, 255, 255);
    break; // WHITE
  }

  matrix.fillScreen(color);
  matrix.show();
}
} // namespace RGBMatrix