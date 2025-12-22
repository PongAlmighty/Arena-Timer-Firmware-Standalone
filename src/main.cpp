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

// Create timer display in TIMER mode (countdown)
TimerDisplay timerDisplay(RGBMatrix::getMatrix(), TimerDisplay::Mode::TIMER);

// Create WebSocket client for FightTimer integration
WebSocketClient *wsClient = nullptr;

void setup() {
  Serial.begin(115200);
  unsigned long start_serial = millis();
  while (!Serial && millis() - start_serial < 3000)
    ; // Wait up to 3s for Serial

  Serial.println("\n\n========================================");
  Serial.println("BOOT: Firmware V2 Starting");
  Serial.println("========================================");
  Serial.println("[BOOT] Arduino Setup Starting...");
  DEBUG_PRINTLN("=== DISPLAY DIAGNOSTIC MODE ===");

  // Initialize RGB Matrix immediately (Bypassing Ethernet for isolation)
  Serial.println("Initializing RGB Matrix...");
  RGBMatrix::init();
  Serial.println("Matrix init() completed");

  RGBMatrix::setOrientation(180);
  Serial.println("Orientation set to 180");

  RGBMatrix::clear();
  Serial.println("Matrix cleared");

  // Try to manually draw a pixel
  Serial.println("Attempting to draw test pixel at (0,0)...");
  auto &m = RGBMatrix::getMatrix();
  m.drawPixel(0, 0, m.color565(255, 0, 0)); // Red pixel at top-left
  m.show();
  Serial.println("Test pixel drawn and show() called");

  pinMode(25, OUTPUT);

  /*
  // Minimal Pixel Test Removed - Restoring Normal App Flow
  DEBUG_PRINTLN("Matrix initialized. Entering normal loop...");
  */
}

void loop() {
  // Handle incoming web requests (includes mDNS update)
  WebServer::handleClient(timerDisplay);

  // Poll WebSocket client for messages
  if (wsClient) {
    wsClient->poll();
  }

  // Clear the display
  RGBMatrix::clear();

  // Update and draw the timer
  timerDisplay.update();

  // Show the display
  RGBMatrix::show();

  // Print status to serial for debugging (every 5 seconds)
  static unsigned long last_serial = 0;
  if (millis() - last_serial >= 5000) {
    Timer::Components remaining = timerDisplay.getTimer().getRemainingTime();
    DEBUG_PRINT("Status: ");
    if (timerDisplay.getTimer().isExpired()) {
      DEBUG_PRINT("EXPIRED ");
    } else if (timerDisplay.getTimer().isRunning()) {
      DEBUG_PRINT("RUNNING ");
    } else if (timerDisplay.getTimer().isPaused()) {
      DEBUG_PRINT("PAUSED ");
    } else if (timerDisplay.getTimer().isIdle()) {
      DEBUG_PRINT("IDLE ");
    }

    DEBUG_PRINT("| Time: ");
    DEBUG_PRINT(remaining.minutes);
    DEBUG_PRINT(":");
    if (remaining.seconds < 10)
      DEBUG_PRINT("0");
    DEBUG_PRINT(remaining.seconds);
    DEBUG_PRINT(".");
    if (remaining.milliseconds < 100)
      DEBUG_PRINT("0");
    if (remaining.milliseconds < 10)
      DEBUG_PRINT("0");
    DEBUG_PRINTLN(remaining.milliseconds);

    last_serial = millis();
  }
}
