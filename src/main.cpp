
#include "ConfigManager.h"
#include "RGBMatrix.h"
#include "TimerDisplay.h"
#include "WebServer.h"
#include "WebSocketClient.h"
#include <Arduino.h>
#include <SPI.h>

// ========================================
// Global Configuration
// ========================================

// Network settings
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
uint8_t static_ip[] = {10, 0, 0, 21};
const char *hostname = "arenatimer";

// Global objects (pointers to be initialized in setup)
TimerDisplay *timerDisplay = nullptr;
WebSocketClient *wsClient = nullptr;

// ========================================
// Setup
// ========================================
void setup() {
  Serial.begin(115200);
  delay(2000); // Wait for serial
  Serial.println("=== Arena Timer Firmware (Standalone) ===");
  Serial.println("Restored Version: " __DATE__ " " __TIME__);

  Serial.println("Initializing Hardware (RGB Matrix)...");
  RGBMatrix::init();
  Adafruit_Protomatter &matrix = RGBMatrix::getMatrix();
  Serial.println("CHECKPOINT: Pre-TimerDisplay");
  timerDisplay = new TimerDisplay(matrix, TimerDisplay::Mode::TIMER);
  Serial.println("CHECKPOINT: Post-TimerDisplay");

  // Show Boot Message
  matrix.fillScreen(0);
  matrix.setCursor(0, 0);
  matrix.setTextColor(matrix.color565(50, 50, 50));
  Serial.println("CHECKPOINT: Pre-Print");
  matrix.print("INIT (CFG ONLY)");
  Serial.println("CHECKPOINT: Post-Print");
  matrix.show();
  Serial.println("CHECKPOINT: Post-Show");

  // Set defaults
  timerDisplay->getTimer().setDuration({3, 0, 0});

  // Initialize ConfigManager
  Serial.println("Loading Configuration...");
  Serial.println("CHECKPOINT: Pre-Config");
  ConfigManager configManager;
  configManager.begin();
  if (!configManager.load()) {
    Serial.println("Failed to load config, using defaults");
  }
  Serial.println("CHECKPOINT: Post-Config");

  // Initialize Network
  Serial.println("Initializing Network...");
  if (WebServer::init(mac, static_ip)) {
    WebServer::initMDNS(hostname);
    WebServer::startWebServer(80);

    // Show IP on Matrix (Non-blocking mode)
    // timerDisplay->setIP(Ethernet.localIP());
    // timerDisplay->setMode(TimerDisplay::Mode::SHOW_IP);
    Serial.print("IP Address: ");
    Serial.println(Ethernet.localIP());

  } else {
    Serial.println("ERROR: Network initialization failed!");
    matrix.fillScreen(0);
    matrix.setCursor(0, 0);
    matrix.setTextColor(matrix.color565(255, 0, 0));
    matrix.print("NET ERR");
    matrix.show();
    delay(2000);
  }

  // Initialize WebSocket Client (for FightTimer integration)
  // Pass the timer pointer so it can control/sync the timer
  wsClient = new WebSocketClient(&timerDisplay->getTimer());

  // Register WebSocket client with WebServer so API can control it
  WebServer::setWebSocketClient(wsClient);

  Serial.println("=== Setup Complete ===");
}

// ========================================
// Main Loop
// ========================================
void loop() {
  // 1. Update Timer Display logic
  if (timerDisplay) {
    timerDisplay->update();
  }

  /* NETWORK DISABLED
  // 2. Handle Web Server clients
  if (timerDisplay) {
    WebServer::handleClient(*timerDisplay);
  }

  // 3. Handle WebSocket events
  if (wsClient) {
    wsClient->poll();
  }

  // 4. Keep Ethernet stack alive (DHCP lease renewals etc)
  Ethernet.maintain();
  */
}
