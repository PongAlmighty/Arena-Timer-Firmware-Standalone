#include <Arduino.h>
#include <RGBMatrix.h>
#include <TimerDisplay.h>
#include <WebServer.h>
#include <WebSocketClient.h>

// Debug flag - set to false to disable debug messages for better timing
#define DEBUG_MAIN true

// Debug printing macros
#if DEBUG_MAIN
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// Include various fonts (12pt and below for 64x32 display)
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
// Retro/pixel fonts
#include <Fonts/Org_01.h>
#include <Fonts/Picopixel.h>
#include <Fonts/TomThumb.h>
// Custom fonts
#include <CustomFonts/AquireBold_8Ma6012pt7b.h>
#include <CustomFonts/AquireLight_YzE0o12pt7b.h>
#include <CustomFonts/Aquire_BW0ox12pt7b.h>

// Network configuration
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
uint8_t static_ip[] = {10, 0, 0, 21}; // Static IP address
const char *hostname = "arenatimer";  // Access via http://arenatimer.local

// Timer display will be created in setup() after matrix is initialized
TimerDisplay *timerDisplay = nullptr;

// Status tracking for diagnosis
String last_setup_step = "READY";
int protomatter_status = -99;

// Create WebSocket client for FightTimer integration
WebSocketClient *wsClient = nullptr;

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 5; i++) {
    delay(500);
    Serial.println("STILL BOOTING...");
  }

  Serial.println("\n\n========================================");
  Serial.println("PHASE 7: RAW GPIO PIN TOGGLE");
  Serial.println("========================================");
  Serial.println("Manual toggle of ALL Matrix pins at 10Hz.");
  Serial.println("Measure pins directly with Multimeter/Scope.");

  // Initialize ALL Matrix pins as outputs
  int allPins[] = {16, 17, 20, 6, 21, 25, 22, 14, 15, 29, 28, 27, 26};
  for (int p : allPins) {
    pinMode(p, OUTPUT);
  }
}

void loop() {
  static bool state = false;
  state = !state;

  int allPins[] = {16, 17, 20, 6, 21, 25, 22, 14, 15, 29, 28, 27, 26};
  for (int p : allPins) {
    digitalWrite(p, state ? HIGH : LOW);
  }

  static unsigned long last_heartbeat = 0;
  if (millis() - last_heartbeat >= 1000) {
    Serial.print("RAW TOGGLE: ");
    Serial.println(state ? "HIGH" : "LOW");
    last_heartbeat = millis();
  }

  delay(100); // 10Hz toggle
}
