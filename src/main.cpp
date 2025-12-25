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

void setup() {
  Serial.begin(115200);

  // Wait for serial with timeout (don't wait forever if no USB connected)
  unsigned long start = millis();
  while (!Serial && (millis() - start < 3000)) {
    delay(10);
  }

  Serial.println("\n\n=== Protomatter + Ethernet Test ===");
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

  Serial.println("\n=== Setup Complete ===\n");
}

void loop() {
  // Simple color cycle test
  static int phase = 0;
  static unsigned long lastChange = 0;

  if (millis() - lastChange > 1000) {
    lastChange = millis();
    phase = (phase + 1) % 4;

    matrix.fillScreen(0); // Clear

    switch (phase) {
    case 0:
      matrix.fillScreen(matrix.color565(255, 0, 0)); // Red
      Serial.println("RED");
      break;
    case 1:
      matrix.fillScreen(matrix.color565(0, 255, 0)); // Green
      Serial.println("GREEN");
      break;
    case 2:
      matrix.fillScreen(matrix.color565(0, 0, 255)); // Blue
      Serial.println("BLUE");
      break;
    case 3:
      matrix.setCursor(4, 12);
      matrix.setTextColor(matrix.color565(255, 255, 255));
      matrix.print("OK!");
      Serial.println("TEXT: OK!");
      break;
    }

    matrix.show(); // Push buffer to display
  }

  // Handle Ethernet (just respond with status)
  EthernetClient client = server.available();
  if (client) {
    Serial.println(">> HTTP Client connected!");
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("Matrix + Ethernet OK");
    client.stop();
    Serial.println("<< Client disconnected");
  }
}
