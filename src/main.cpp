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

  matrix.fillScreen(0);
  timerDisplay.getTimer().setDuration(Timer::Components{3, 0, 0});
  timerDisplay.addColorThreshold(60, 255, 0, 0);
  timerDisplay.addColorThreshold(120, 255, 255, 0);
}

// ----------------------------------------------------------------------------
// LOOP
// ----------------------------------------------------------------------------
void loop() {
  timerDisplay.update();
  Ethernet.maintain();
  WebServer::handleClient(timerDisplay);
  if (wsClient) {
    wsClient->poll();
  }
}
