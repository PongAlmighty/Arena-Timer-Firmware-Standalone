#include "SimpleWebSocketClient.h"
#include <Adafruit_Protomatter.h>
#include <Arduino.h>
#include <Ethernet_Generic.h>
#include <SPI.h>

// Matrix pin definitions - based on ACTUAL physical wiring
// HUB75 connector wired to RP2040-Shim
uint8_t rgbPins[] = {16, 17, 20, 6, 19, 25}; // R1, G1, B1, R2, G2, B2
uint8_t addrPins[] = {29, 28, 27, 26};       // A, B, C, D
uint8_t clockPin = 22;
uint8_t latchPin = 1; // IDC pin 14 wired to RX (GPIO 1)
uint8_t oePin = 0;    // IDC pin 15 wired to TX (GPIO 0)

Adafruit_Protomatter
    matrix(64,       // Width
           4,        // Bit depth (4 = 16 shades per color = 4096 colors)
           1,        // RGB count (1 set of RGB pins)
           rgbPins,  // RGB pin list
           4,        // Address pin count
           addrPins, // Address pins
           clockPin, latchPin, oePin,
           false // Double buffer (false = less RAM)
    );

// Ethernet - W5500 on SPI1
// SPI1 hardware pins for RP2040-Shim + POE-FeatherWing:
//   SCK  = GPIO 10
//   MOSI = GPIO 11 (TX)
//   MISO = GPIO 12 (RX)
//   CS   = GPIO 21
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(172, 17, 17, 167);
EthernetServer server(80);

// WebSocket client for FightTimer connection
EthernetClient wsEthClient;
SimpleWebSocketClient wsClient(wsEthClient);

// FightTimer server settings
const char *FIGHTTIMER_HOST = "fighttimer.local";
const uint16_t FIGHTTIMER_PORT = 8765;
const char *FIGHTTIMER_PATH = "/socket.io/?EIO=4&transport=websocket";

// Timer state (received from server)
int timerMinutes = 3;
int timerSeconds = 0;
bool timerRunning = false;
bool timerExpired = false;
String displayMessage = "";

// Display update tracking
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 100; // 100ms update rate

// WebSocket message handler
void onWebSocketMessage(const char *message, size_t length) {
  Serial.print("[Timer] Received: ");
  Serial.println(message);

  // Parse Socket.IO event: ["timer_update", {...}]
  // For now, just display that we received a message
  // TODO: Add JSON parsing for timer_update events

  // Simple parse: look for "timer_update" and extract action
  String msg(message);
  if (msg.indexOf("timer_update") >= 0) {
    Serial.println("[Timer] Timer update event received");

    // Look for action
    int actionPos = msg.indexOf("\"action\":");
    if (actionPos > 0) {
      int startQuote = msg.indexOf('"', actionPos + 9);
      int endQuote = msg.indexOf('"', startQuote + 1);
      if (startQuote > 0 && endQuote > startQuote) {
        String action = msg.substring(startQuote + 1, endQuote);
        Serial.print("[Timer] Action: ");
        Serial.println(action);

        if (action == "start") {
          timerRunning = true;
          timerExpired = false;
        } else if (action == "stop") {
          timerRunning = false;
        } else if (action == "reset") {
          timerRunning = false;
          timerExpired = false;
          // Try to extract minutes/seconds
          int minPos = msg.indexOf("\"minutes\":");
          if (minPos > 0) {
            int numStart = minPos + 10;
            int numEnd = numStart;
            while (numEnd < (int)msg.length() &&
                   (isDigit(msg[numEnd]) || msg[numEnd] == '-'))
              numEnd++;
            timerMinutes = msg.substring(numStart, numEnd).toInt();
          }
          int secPos = msg.indexOf("\"seconds\":");
          if (secPos > 0) {
            int numStart = secPos + 10;
            int numEnd = numStart;
            while (numEnd < (int)msg.length() &&
                   (isDigit(msg[numEnd]) || msg[numEnd] == '-'))
              numEnd++;
            timerSeconds = msg.substring(numStart, numEnd).toInt();
          }
          Serial.print("[Timer] Reset to ");
          Serial.print(timerMinutes);
          Serial.print(":");
          if (timerSeconds < 10)
            Serial.print("0");
          Serial.println(timerSeconds);
        }
      }
    }
  }
}

void displayTimer() {
  matrix.fillScreen(0); // Clear

  // Choose color based on timer state
  uint16_t color;
  if (timerExpired) {
    color = matrix.color565(255, 0, 0); // Red when expired
  } else if (timerMinutes == 0 && timerSeconds <= 30) {
    color = matrix.color565(255, 255, 0); // Yellow when < 30s
  } else {
    color = matrix.color565(0, 255, 0); // Green normally
  }

  // Format time string
  char timeStr[8];
  snprintf(timeStr, sizeof(timeStr), "%d:%02d", timerMinutes, timerSeconds);

  // Center text (approximate - depends on font)
  int16_t x = 10;
  int16_t y = 12;

  matrix.setTextColor(color);
  matrix.setTextSize(2);
  matrix.setCursor(x, y);
  matrix.print(timeStr);

  matrix.show();
}

void displayStatus(const char *status) {
  matrix.fillScreen(0);
  matrix.setTextColor(matrix.color565(255, 255, 255));
  matrix.setTextSize(1);
  matrix.setCursor(2, 12);
  matrix.print(status);
  matrix.show();
}

void setup() {
  Serial.begin(115200);

  // Wait for serial with timeout (don't wait forever if no USB connected)
  unsigned long start = millis();
  while (!Serial && (millis() - start < 3000)) {
    delay(10);
  }

  Serial.println("\n\n=== Arena Timer - FightTimer Client ===");
  Serial.println("Firmware compiled: " __DATE__ " " __TIME__);

  // ========================================
  // ETHERNET INIT (before matrix, as it uses SPI1)
  // ========================================
  Serial.println("\n--- Ethernet Setup ---");

  // Configure SPI1 pins explicitly for RP2040-Shim
  Serial.println("Configuring SPI1 pins: SCK=10, TX=11, RX=12");
  SPI1.setSCK(10);
  SPI1.setTX(11);
  SPI1.setRX(12);
  SPI1.begin();

  Serial.println("Setting Ethernet CS pin to GPIO 21");
  Ethernet.init(21); // CS pin

  Serial.println("Calling Ethernet.begin()...");
  Ethernet.begin(mac, ip);

  Serial.print("Ethernet hardware status: ");
  switch (Ethernet.hardwareStatus()) {
  case EthernetNoHardware:
    Serial.println("NO HARDWARE DETECTED!");
    break;
  case EthernetW5100:
    Serial.println("W5100");
    break;
  case EthernetW5200:
    Serial.println("W5200");
    break;
  case EthernetW5500:
    Serial.println("W5500");
    break;
  case EthernetW5100S:
    Serial.println("W5100S");
    break;
  default:
    Serial.println("Unknown");
  }

  Serial.print("Link status: ");
  switch (Ethernet.linkStatus()) {
  case LinkON:
    Serial.println("ON");
    break;
  case LinkOFF:
    Serial.println("OFF");
    break;
  default:
    Serial.println("Unknown");
  }

  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());

  server.begin();
  Serial.println("HTTP Server started on port 80");

  // ========================================
  // MATRIX INIT
  // ========================================
  Serial.println("\n--- Matrix Setup ---");
  Serial.println("Initializing Protomatter...");

  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter status: ");

  switch (status) {
  case PROTOMATTER_OK:
    Serial.println("OK");
    break;
  case PROTOMATTER_ERR_PINS:
    Serial.println("ERROR - RGB+CLK pins not on same PORT");
    while (1)
      ;
    break;
  case PROTOMATTER_ERR_MALLOC:
    Serial.println("ERROR - Memory allocation failed");
    while (1)
      ;
    break;
  case PROTOMATTER_ERR_ARG:
    Serial.println("ERROR - Bad argument");
    while (1)
      ;
    break;
  default:
    Serial.println("ERROR - Unknown");
    while (1)
      ;
  }

  // ========================================
  // WEBSOCKET CLIENT INIT
  // ========================================
  Serial.println("\n--- WebSocket Setup ---");

  // Configure WebSocket client
  wsClient.setSocketIOMode(true);        // Enable Socket.IO protocol
  wsClient.setAutoReconnect(true, 5000); // Auto-reconnect every 5 seconds
  wsClient.onMessage(onWebSocketMessage);

  Serial.print("Connecting to FightTimer: ");
  Serial.print(FIGHTTIMER_HOST);
  Serial.print(":");
  Serial.println(FIGHTTIMER_PORT);

  // Initial display
  displayStatus("Connecting...");

  // Try to connect (will auto-reconnect if fails)
  wsClient.connect(FIGHTTIMER_HOST, FIGHTTIMER_PORT, FIGHTTIMER_PATH);

  Serial.println("\n=== Setup Complete ===\n");
}

void loop() {
  // Poll WebSocket for incoming messages
  wsClient.poll();

  // Update display periodically
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = millis();

    if (wsClient.isConnected()) {
      displayTimer();
    } else {
      // Show connection status
      static int dots = 0;
      dots = (dots + 1) % 4;
      char statusMsg[20];
      snprintf(statusMsg, sizeof(statusMsg), "Connect%.*s", dots, "...");
      displayStatus(statusMsg);
    }
  }

  // Handle HTTP server (for status/debugging)
  EthernetClient client = server.available();
  if (client) {
    Serial.println(">> HTTP Client connected!");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.print("Arena Timer Status\n");
    client.print("==================\n");
    client.print("WebSocket: ");
    client.println(wsClient.isConnected() ? "Connected" : "Disconnected");
    client.print("Server: ");
    client.println(wsClient.getServerUrl());
    client.print("Timer: ");
    client.print(timerMinutes);
    client.print(":");
    if (timerSeconds < 10)
      client.print("0");
    client.println(timerSeconds);
    client.print("Running: ");
    client.println(timerRunning ? "Yes" : "No");
    client.stop();
    Serial.println("<< Client disconnected");
  }

  // Maintain Ethernet connection
  Ethernet.maintain();
}
