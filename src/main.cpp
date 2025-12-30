#include "TimerDisplay.h"
#include "WebServer.h"
#include "WebSocketClient.h"
#include <Adafruit_Protomatter.h>
#include <Arduino.h>
#include <Ethernet_Generic.h>
#include <SPI.h>

// ----------------------------------------------------------------------------
// HARDWARE PIN CONFIGURATION (Verified)
// ----------------------------------------------------------------------------

// RGB Matrix Pins (RP2040-Shim / SPI0)
uint8_t rgbPins[] = {16, 17, 20, 6, 19, 25};
uint8_t addrPins[] = {29, 28, 27, 26};
uint8_t clockPin = 22;
uint8_t latchPin = 1;
uint8_t oePin = 0;

// Ethernet Pins (W5500 / SPI1)
#define ETH_SCK 10
#define ETH_TX 11
#define ETH_RX 12
#define ETH_CS 21

// Button Pins (Available on RP2040-Shim headers)
#define BTN_START 7    // D5
#define BTN_STOP 2     // A4 (also Reset on long press)
#define BTN_NET_INFO 3 // A5

// ----------------------------------------------------------------------------
// GLOBAL OBJECTS
// ----------------------------------------------------------------------------

Adafruit_Protomatter matrix(64, 4, 1, rgbPins, 4, addrPins, clockPin, latchPin,
                            oePin, true);

TimerDisplay timerDisplay(matrix);
WebSocketClient *wsClient = nullptr;

// Network Config defaults
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
uint8_t ip[] = {10, 0, 0, 21}; // Fallback static IP
const char *hostname = "arenatimer";

// ----------------------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Arena Timer Booting ===");

  // 1. Matrix Init
  ProtomatterStatus status = matrix.begin();
  Serial.print("Matrix Status: ");
  Serial.println((int)status);

  matrix.fillScreen(0);
  matrix.setCursor(0, 0);
  matrix.setTextColor(matrix.color565(0, 0, 255));
  matrix.println("Arena");
  matrix.println("Timer");
  matrix.show();

  // 2. Ethernet Hardware Init (SPI1)
  SPI1.setSCK(ETH_SCK);
  SPI1.setTX(ETH_TX);
  SPI1.setRX(ETH_RX);
  SPI1.begin();

  Ethernet.init(ETH_CS);

  // 3. Network Logic Init
  Serial.print("Initializing Network...");
  timerDisplay.showMessage("DHCP...");
  if (WebServer::init(mac, ip)) {
    Serial.println("OK");
    WebServer::startWebServer(80);
    WebServer::initMDNS(hostname);

    Serial.print("IP: ");
    Serial.println(Ethernet.localIP());
    timerDisplay.showMessage(String(Ethernet.localIP()[0]) + "." +
                             String(Ethernet.localIP()[1]) + "." +
                             String(Ethernet.localIP()[2]) + "." +
                             String(Ethernet.localIP()[3]));
    matrix.show();
    delay(2000);
  } else {
    Serial.println("FAIL");
    matrix.fillScreen(0);
    matrix.setTextColor(matrix.color565(255, 0, 0));
    matrix.println("Net Err");
    matrix.show();
  }

  // 4. WebSocket Init
  wsClient = new WebSocketClient(&timerDisplay.getTimer());
  WebServer::setWebSocketClient(wsClient);

  // 5. Load Persistent Settings
  Serial.print("Loading saved settings...");
  if (WebServer::loadSettings(timerDisplay)) {
    Serial.println("OK");
  } else {
    Serial.println("Using defaults");
    // Default initial setup if no settings exist
    timerDisplay.getTimer().setDuration(Timer::Components{3, 0, 0});
    timerDisplay.clearColorThresholds();
    timerDisplay.addColorThreshold(60, 255, 0, 0);
    timerDisplay.addColorThreshold(120, 255, 255, 0);
  }

  matrix.fillScreen(0);

  // 6. Initialize Buttons
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);
  pinMode(BTN_NET_INFO, INPUT_PULLUP);
}

// ----------------------------------------------------------------------------
// LOOP
// ----------------------------------------------------------------------------
void loop() {
  static unsigned long last_btn_ms = 0;
  static unsigned long stop_press_start_ms = 0;
  static bool was_stop_pressed = false;

  // 1. Check Network Info Button (Highest priority, non-blocking hold)
  if (digitalRead(BTN_NET_INFO) == LOW) {
    timerDisplay.renderNetworkStatus();
  } else {
    // 2. Normal Timer Update
    timerDisplay.update();

    // 3. Handle Start Button (Debounced)
    if (digitalRead(BTN_START) == LOW && (millis() - last_btn_ms > 300)) {
      if (!timerDisplay.getTimer().isRunning()) {
        Serial.println("Physical Button: START");
        timerDisplay.getTimer().start();
      }
      last_btn_ms = millis();
    }

    // 4. Handle Stop/Reset Button (Short press = Stop, Long press = Reset)
    bool stop_currently_pressed = (digitalRead(BTN_STOP) == LOW);
    if (stop_currently_pressed && !was_stop_pressed) {
      // Button just pressed
      stop_press_start_ms = millis();
      was_stop_pressed = true;
    } else if (!stop_currently_pressed && was_stop_pressed) {
      // Button just released
      unsigned long press_duration = millis() - stop_press_start_ms;
      if (press_duration > 2000) {
        // Long press (>2s) = RESET
        Serial.println("Physical Button: RESET (Long Press)");
        timerDisplay.getTimer().reset();
      } else if (press_duration > 50) {
        // Short press = STOP
        if (timerDisplay.getTimer().isRunning()) {
          Serial.println("Physical Button: STOP");
          timerDisplay.getTimer().stop();
        }
      }
      was_stop_pressed = false;
      last_btn_ms = millis();
    }
  }

  Ethernet.maintain();
  WebServer::handleClient(timerDisplay);
  if (wsClient) {
    wsClient->poll();
  }
}
