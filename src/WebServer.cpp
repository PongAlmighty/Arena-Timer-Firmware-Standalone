#include "WebServer.h"
// #include "RGBMatrix.h"
#include "WebSocketClient.h"
#include <ArduinoJson.h>
#include <EthernetBonjour.h>
#include <LittleFS.h>
#include <SPI.h>

// Debug flag - set to false to disable debug messages for better timing
#define DEBUG_WEBSERVER true

// Debug printing macros
#if DEBUG_WEBSERVER
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// Font includes (12pt and below for 64x32 display)
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

namespace WebServer {
// Pin definitions are handled by main.cpp/platformio.ini now
// But we need to know CS for Ethernet.init() if we call it here.
// However, main.cpp calls Ethernet.init(21), so we might not need to do it
// here. But as a safeguard, we will verify. In this architecture, main.cpp
// handles hardware init. This file handles LOGIC.

EthernetServer *server = nullptr;
bool mdns_initialized = false;
WebSocketClient *wsClient = nullptr;
int current_orientation = 180; // Track current display orientation

bool init(uint8_t mac[6], uint8_t ip[4]) {
  // Initialize LittleFS - format if mount fails (e.g. first boot with 1M quota)
  if (!LittleFS.begin()) {
    DEBUG_PRINTLN("LittleFS mount failed, attempting to format...");
    if (LittleFS.format()) {
      DEBUG_PRINTLN("LittleFS formatted successfully");
      if (LittleFS.begin()) {
        DEBUG_PRINTLN("LittleFS mounted after format");
      } else {
        DEBUG_PRINTLN("ERROR: LittleFS mount failed even after format");
      }
    } else {
      DEBUG_PRINTLN("ERROR: LittleFS format failed");
    }
  } else {
    DEBUG_PRINTLN("LittleFS initialized successfully");
    // Show stats
    FSInfo info;
    if (LittleFS.info(info)) {
      DEBUG_PRINT("FS Total: ");
      DEBUG_PRINT(info.totalBytes);
      DEBUG_PRINT(" Used: ");
      DEBUG_PRINTLN(info.usedBytes);
    }
  }

  // HARDWARE INIT REMOVED - Handled by main.cpp / platformio.ini
  // No SPI.setSCK etc.

  // Try DHCP first, fall back to static IP if DHCP fails
  DEBUG_PRINTLN("Attempting DHCP...");

  // Pass &SPI1 because we are using the non-default SPI instance
  // Note: Ethernet_Generic 2.8.1 supports begin(mac, startTimeout, &SPI) for
  // DHCP
  if (Ethernet.begin(mac, &SPI1) == 0) {
    DEBUG_PRINTLN("DHCP failed, using static IP");
    // Fallback to static IP
    // For static IP without specific gateway/dns: begin(mac, ip)
    Ethernet.begin(mac, ip);
  } else {
    DEBUG_PRINTLN("DHCP successful");
  }

  // Give Ethernet time to initialize and establish link
  delay(2000);

  // Check if Ethernet is connected
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    DEBUG_PRINTLN("ERROR: Ethernet hardware not found");
    return false;
  }

  if (Ethernet.linkStatus() == LinkOFF) {
    DEBUG_PRINTLN("WARNING: Ethernet cable not connected");
  }

  DEBUG_PRINT("Ethernet initialized - IP: ");
  DEBUG_PRINTLN(Ethernet.localIP());

  return true;
}

bool initMDNS(const char *hostname) {
  if (!EthernetBonjour.begin(hostname)) {
    DEBUG_PRINTLN("ERROR: Failed to start mDNS responder");
    mdns_initialized = false;
    return false;
  }

  DEBUG_PRINT("mDNS responder started: ");
  DEBUG_PRINT(hostname);
  DEBUG_PRINTLN(".local");
  mdns_initialized = true;
  return true;
}

void updateMDNS() {
  if (mdns_initialized) {
    EthernetBonjour.run();
  }
}

String getIPAddressString() {
  IPAddress ip = Ethernet.localIP();
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." +
         String(ip[3]);
}

void startWebServer(uint16_t port) {
  if (server != nullptr) {
    delete server;
  }
  server = new EthernetServer(port);
  server->begin();
  DEBUG_PRINT("Web server started on port ");
  DEBUG_PRINTLN(port);
}

EthernetServer &getServer() { return *server; }

void setWebSocketClient(WebSocketClient *client) {
  wsClient = client;
  DEBUG_PRINTLN("WebSocket client registered with WebServer");
}

// Helper function to send HTTP response
void sendHTTPResponse(EthernetClient &client, int code, const char *contentType,
                      const String &body) {
  client.print("HTTP/1.1 ");
  client.print(code);
  client.println(code == 200 ? " OK" : " Error");
  client.print("Content-Type: ");
  client.println(contentType);
  client.println("Connection: close");
  client.println();
  client.print(body);
}

// Helper function to URL decode a string
String urlDecode(const String &str) {
  String decoded = "";
  char temp[] = "00";
  for (unsigned int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (c == '%' && i + 2 < str.length()) {
      temp[0] = str.charAt(i + 1);
      temp[1] = str.charAt(i + 2);
      decoded += (char)strtol(temp, NULL, 16);
      i += 2;
    } else if (c == '+') {
      decoded += ' ';
    } else {
      decoded += c;
    }
  }
  return decoded;
}

// Helper function to parse hex color string to RGB
void parseColor(const String &hexColor, uint8_t &r, uint8_t &g, uint8_t &b) {
  String color = hexColor;
  if (color.startsWith("#")) {
    color = color.substring(1);
  }

  long number = strtol(color.c_str(), NULL, 16);
  r = (number >> 16) & 0xFF;
  g = (number >> 8) & 0xFF;
  b = number & 0xFF;
}

// Helper function to get font pointer from font ID
const GFXfont *getFontById(int fontId) {
  switch (fontId) {
  case 0:
    return nullptr; // Default font (5x7 pixels)
  // Sans-Serif fonts
  case 1:
    return &FreeSans9pt7b;
  case 2:
    return &FreeSans12pt7b;
  case 3:
    return &FreeSansBold9pt7b;
  case 4:
    return &FreeSansBold12pt7b;
  // Monospace fonts
  case 5:
    return &FreeMono9pt7b;
  case 6:
    return &FreeMono12pt7b;
  case 7:
    return &FreeMonoBold9pt7b;
  case 8:
    return &FreeMonoBold12pt7b;
  // Serif fonts
  case 9:
    return &FreeSerif9pt7b;
  case 10:
    return &FreeSerif12pt7b;
  case 11:
    return &FreeSerifBold9pt7b;
  case 12:
    return &FreeSerifBold12pt7b;
  // Retro/Pixel fonts
  case 13:
    return &Org_01;
  case 14:
    return &Picopixel;
  case 15:
    return &TomThumb;
  // Custom fonts
  case 16:
    return &Aquire_BW0ox12pt7b;
  case 17:
    return &AquireBold_8Ma6012pt7b;
  case 18:
    return &AquireLight_YzE0o12pt7b;
  default:
    return &FreeSansBold12pt7b; // Default to 12pt bold
  }
}

// Helper function to get text size for font ID
uint8_t getTextSizeForFont(int fontId) {
  // Default font and retro fonts use larger scaling
  if (fontId == 0)
    return 2; // Default 5x7 @ 2x
  if (fontId >= 13 && fontId <= 15)
    return 3; // Retro fonts @ 3x (they're very small)
  return 1;   // All other fonts @ 1x
}

bool saveSettings(TimerDisplay &timerDisplay) {
  JsonDocument doc;
  doc["duration"] = timerDisplay.getTimer().getDurationSeconds();
  doc["font"] = timerDisplay.getFontId();
  doc["spacing"] = timerDisplay.getLetterSpacing();
  doc["brightness"] = timerDisplay.getBrightness();

  JsonArray thresholds = doc["thresholds"].to<JsonArray>();
  size_t count = 0;
  const TimerDisplay::ColorThreshold *data =
      timerDisplay.getColorThresholds(count);
  if (data != nullptr) {
    for (size_t i = 0; i < count; i++) {
      JsonObject t = thresholds.add<JsonObject>();
      t["seconds"] = data[i].seconds;
      char colorHex[8];
      snprintf(colorHex, sizeof(colorHex), "#%02X%02X%02X", data[i].r,
               data[i].g, data[i].b);
      t["color"] = String(colorHex); // Robust copy
    }
  }

  uint8_t dr, dg, db;
  timerDisplay.getDefaultColor(dr, dg, db);
  char defaultHex[8];
  snprintf(defaultHex, sizeof(defaultHex), "#%02X%02X%02X", dr, dg, db);
  doc["defaultColor"] = defaultHex;

  File file = LittleFS.open("/settings.json", "w");
  if (!file) {
    DEBUG_PRINTLN("ERROR: Failed to open /settings.json for writing");
    return false;
  }

  size_t bytes = serializeJson(doc, file);
  file.close();

  if (bytes == 0) {
    DEBUG_PRINTLN("ERROR: Failed to write JSON to /settings.json");
    return false;
  }

  DEBUG_PRINT("Settings saved to /settings.json (");
  DEBUG_PRINT(bytes);
  DEBUG_PRINTLN(" bytes)");
  return true;
}

bool loadSettings(TimerDisplay &timerDisplay) {
  if (!LittleFS.exists("/settings.json")) {
    DEBUG_PRINTLN("No settings file found");
    return false;
  }

  File file = LittleFS.open("/settings.json", "r");
  if (!file) {
    DEBUG_PRINTLN("Failed to open settings file for reading");
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    DEBUG_PRINTLN("Failed to parse settings file");
    return false;
  }

  if (!doc["duration"].isNull()) {
    int duration = doc["duration"];
    Timer::Components comp;
    comp.minutes = duration / 60;
    comp.seconds = duration % 60;
    comp.milliseconds = 0;
    timerDisplay.getTimer().setDuration(comp);
    timerDisplay.getTimer().reset();
  }

  if (!doc["font"].isNull()) {
    int fontId = doc["font"];
    timerDisplay.setFont(getFontById(fontId), fontId);
    timerDisplay.setTextSize(getTextSizeForFont(fontId));
  }

  if (!doc["spacing"].isNull()) {
    timerDisplay.setLetterSpacing(doc["spacing"]);
  }

  if (!doc["brightness"].isNull()) {
    timerDisplay.setBrightness(doc["brightness"]);
  }

  if (!doc["thresholds"].isNull()) {
    timerDisplay.clearColorThresholds();
    JsonArray thresholds = doc["thresholds"];
    int count = 0;
    for (JsonVariant v : thresholds) {
      JsonObject t = v.as<JsonObject>();
      if (!t.isNull()) {
        unsigned int seconds = t["seconds"];
        const char *color = t["color"];
        if (color) {
          uint8_t r, g, b;
          parseColor(color, r, g, b);
          timerDisplay.addColorThreshold(seconds, r, g, b);
          count++;
          DEBUG_PRINT("Threshold Loaded: ");
          DEBUG_PRINT(seconds);
          DEBUG_PRINT("s -> ");
          DEBUG_PRINTLN(color);
        }
      }
    }
    DEBUG_PRINT("Loaded ");
    DEBUG_PRINT(count);
    DEBUG_PRINTLN(" thresholds");
  }

  if (!doc["defaultColor"].isNull()) {
    uint8_t r, g, b;
    parseColor(doc["defaultColor"].as<const char *>(), r, g, b);
    timerDisplay.setDefaultColor(r, g, b);
    timerDisplay.setColor(r, g, b);
  }

  DEBUG_PRINTLN("Settings loaded from /settings.json");
  return true;
}

void handleClient(TimerDisplay &timerDisplay) {
  // Update mDNS responder to keep hostname resolution alive
  updateMDNS();

  if (server == nullptr)
    return;

  EthernetClient client = server->available();
  if (client) {
    String currentLine = "";
    String requestType = "";
    String requestPath = "";
    String postData = "";
    bool isPost = false;
    int contentLength = 0;

    // Read HTTP request
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // End of headers, read POST data if needed
            if (isPost && contentLength > 0) {
              postData.reserve(contentLength);
              unsigned long startTime = millis();
              int bytesRead = 0;
              while (bytesRead < contentLength && millis() - startTime < 1000) {
                if (client.available()) {
                  postData += (char)client.read();
                  bytesRead++;
                }
              }
            }
            break;
          } else {
            // Parse request line
            if (requestType == "") {
              int firstSpace = currentLine.indexOf(' ');
              int secondSpace = currentLine.indexOf(' ', firstSpace + 1);
              if (firstSpace > 0 && secondSpace > firstSpace) {
                requestType = currentLine.substring(0, firstSpace);
                requestPath =
                    currentLine.substring(firstSpace + 1, secondSpace);
                isPost = (requestType == "POST");
              }
            }
            // Check for Content-Length header
            // Check for Content-Length header
            String lineLower = currentLine;
            lineLower.toLowerCase();
            if (lineLower.startsWith("content-length: ")) {
              contentLength = currentLine.substring(16).toInt();
            }
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    // Handle different endpoints
    if (requestPath == "/" || requestPath.startsWith("/?")) {
      // Serve web page
      DEBUG_PRINTLN("Client connected - serving web page");
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();

      // Send HTML in chunks using F() to save RAM
      client.print(
          F("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"));
      client.print(F("<meta name='viewport' content='width=device-width, "
                     "initial-scale=1.0'>"));
      client.print(F("<title>Arena Timer Control</title><style>"));
      client.print(
          F("body{font-family:Arial,sans-serif;margin:0;padding:20px;"));
      client.print(
          F("background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);"));
      client.print(F("min-height:100vh}"));
      client.print(
          F(".container{background:white;border-radius:10px;padding:30px;"));
      client.print(F("box-shadow:0 10px 40px "
                     "rgba(0,0,0,0.2);max-width:1400px;margin:0 auto}"));
      client.print(F("h1{text-align:center;color:#333;margin-bottom:30px}"));
      client.print(F(
          ".grid-container{display:grid;grid-template-columns:repeat(3,1fr);"));
      client.print(F("gap:20px;margin-top:20px}"));
      client.print(
          F("@media "
            "(max-width:1200px){.grid-container{grid-template-columns:1fr}}"));
      client.print(
          F(".section{margin-bottom:25px;padding:20px;background:#f5f5f5;"));
      client.print(F("border-radius:8px}.section "
                     "h2{margin-top:0;color:#667eea;font-size:18px}"));
      client.print(F(".controls{display:grid;grid-template-columns:1fr "
                     "1fr;gap:10px;margin-bottom:15px}"));
      client.print(F("button{padding:15px "
                     "20px;border:none;border-radius:6px;font-size:16px;"));
      client.print(F("cursor:pointer;transition:all 0.3s;font-weight:bold}"));
      client.print(
          F(".btn-start{background:#4CAF50;color:white;grid-column:1/-1}"));
      client.print(F(".btn-start:hover{background:#45a049}"));
      client.print(F(".btn-pause{background:#FF9800;color:white}"));
      client.print(F(".btn-pause:hover{background:#e68900}"));
      client.print(F(".btn-reset{background:#f44336;color:white}"));
      client.print(F(".btn-reset:hover{background:#da190b}"));
      client.print(F(".form-group{margin-bottom:15px}"));
      client.print(F("label{display:block;margin-bottom:5px;color:#555;font-"
                     "weight:bold}"));
      client.print(F("input[type='number'],input[type='color'],select{width:"
                     "100%;padding:10px;"));
      client.print(
          F("border:2px solid "
            "#ddd;border-radius:6px;font-size:14px;box-sizing:border-box}"));
      client.print(F("input[type='number']:focus,input[type='color']:focus,"
                     "select:focus{"));
      client.print(F("border-color:#667eea;outline:none}"));
      client.print(F("input[type='color']{height:45px;cursor:pointer;border-"
                     "radius:6px;min-width:60px}"));
      client.print(F(".threshold-list{margin-bottom:15px}"));
      client.print(
          F(".threshold-item{display:flex;align-items:center;gap:10px;"));
      client.print(F("margin-bottom:10px;padding:12px;background:white;border-"
                     "radius:8px;"));
      client.print(F("border-left:4px solid #667eea;box-shadow:0 2px 4px "
                     "rgba(0,0,0,0.05)}"));
      client.print(F(".threshold-item "
                     ".time-inputs{display:flex;gap:5px;align-items:center;"));
      client.print(F("flex:1;white-space:nowrap}"));
      client.print(
          F(".threshold-item "
            ".when-label{color:#666;font-weight:500;white-space:nowrap}"));
      client.print(
          F(".threshold-item "
            "input[type='number']{width:60px;padding:8px;text-align:center;"));
      client.print(F("font-size:16px;font-weight:bold;flex-shrink:0}"));
      client.print(
          F(".threshold-item "
            ".time-label{font-size:12px;color:#999;font-weight:normal}"));
      client.print(F(
          ".threshold-item .arrow{color:#667eea;font-size:20px;margin:0 8px}"));
      client.print(
          F(".threshold-default{display:flex;align-items:center;gap:10px;"));
      client.print(F("padding:12px;background:white;border-radius:8px;"));
      client.print(F("border-left:4px solid #667eea;box-shadow:0 2px 4px "
                     "rgba(0,0,0,0.05);margin-bottom:10px}"));
      client.print(
          F(".threshold-default .label{flex:1;color:#666;font-weight:500}"));
      client.print(
          F(".duration-card{padding:20px;background:white;border-radius:8px;"));
      client.print(F("box-shadow:0 2px 4px rgba(0,0,0,0.05);margin-top:15px}"));
      client.print(F(".duration-inputs{display:flex;gap:8px;align-items:center;"
                     "margin-top:10px}"));
      client.print(F(".duration-inputs "
                     "input{width:80px;text-align:center;font-size:16px;font-"
                     "weight:bold}"));
      client.print(F(".duration-inputs span{color:#666;font-size:14px}"));
      client.print(F(".btn-remove{background:#ff5252;color:white;padding:8px "
                     "12px;border:none;"));
      client.print(F(
          "border-radius:6px;cursor:pointer;font-size:14px;font-weight:bold;"));
      client.print(F("transition:background 0.2s}"));
      client.print(F(".btn-remove:hover{background:#ff1744}"));
      client.print(F(
          ".btn-add{background:#4CAF50;color:white;padding:12px;border:none;"));
      client.print(F("border-radius:8px;cursor:pointer;width:100%;font-size:"
                     "14px;font-weight:bold;"));
      client.print(F("margin-bottom:15px;transition:background 0.2s}"));
      client.print(F(".btn-add:hover{background:#45a049}"));
      client.print(F(".console{background:#1e1e1e;color:#d4d4d4;padding:15px;"
                     "border-radius:8px;"));
      client.print(F(
          "font-family:'Courier New',monospace;font-size:12px;height:200px;"));
      client.print(
          F("overflow-y:auto;box-shadow:inset 0 2px 4px rgba(0,0,0,0.3)}"));
      client.print(F(".console-entry{margin-bottom:8px;line-height:1.4}"));
      client.print(F(".console-time{color:#858585;margin-right:8px}"));
      client.print(F(".console-success{color:#4CAF50}"));
      client.print(F(".console-error{color:#f44336}"));
      client.print(F(".console-info{color:#2196F3}"));
      client.print(F(".console-warning{color:#FF9800}"));
      client.print(
          F(".info-display{background:white;padding:12px;border-radius:8px;"));
      client.print(F("margin-bottom:15px;border-left:4px solid #667eea;"));
      client.print(F("box-shadow:0 2px 4px rgba(0,0,0,0.05)}"));
      client.print(F(".info-label{color:#666;font-size:12px;font-weight:500;"
                     "text-transform:uppercase}"));
      client.print(F(".info-value{color:#333;font-size:16px;font-weight:bold;"
                     "margin-top:4px;"));
      client.print(F("font-family:monospace}"));
      client.print(F(".apply-button{margin-top:20px;width:100%}"));
      client.print(F(".apply-button.sticky{position:fixed;bottom:20px;left:50%;"
                     "transform:translateX(-50%);"));
      client.print(F("width:300px;max-width:90vw;z-index:1000;box-shadow:0 4px "
                     "15px rgba(0,0,0,0.3)!important}"));
      client.print(F(".content-with-sticky{padding-bottom:80px}"));
      client.print(F("</style></head><body><div class='container'>"));
      client.print(F("<h1>⏱️ Arena Timer Control</h1>"));
      client.print(F("<div class='grid-container'>"));

      // Column 1: Timer Controls & Duration & Console
      client.print(F("<div class='grid-column'>"));
      client.print(F("<div class='section'><h2>🎮 Timer Controls</h2><div "
                     "class='controls'>"));
      client.print(F("<button id='startBtn' class='btn-start' "
                     "onclick='sendCommand(\"start\")'>▶️ Start</button>"));
      client.print(F("<button class='btn-pause' "
                     "onclick='sendCommand(\"pause\")'>⏸️ Pause</button>"));
      client.print(F("<button class='btn-reset' "
                     "onclick='sendCommand(\"reset\")'>🔄 Reset</button>"));
      client.print(F("<button class='btn-pause' onclick='toggleOrientation()' "
                     "style='grid-column:1/-1'>"));
      client.print(F("🔄 Flip Display</button>"));
      client.print(F("</div></div>"));
      client.print(F("<div class='section'><h2>⏲️ Timer Duration</h2>"));
      client.print(F("<div class='duration-inputs'>"));
      client.print(F(
          "<input type='number' id='durationMin' value='3' min='0' max='60'>"));
      client.print(F("<span>min</span>"));
      client.print(F(
          "<input type='number' id='durationSec' value='0' min='0' max='59'>"));
      client.print(F("<span>sec</span></div></div>"));

      // Console Card
      client.print(F("<div class='section'><h2>📝 Console</h2>"));
      client.print(F("<div id='console' class='console'>"));
      client.print(F("<div class='console-entry console-info'>"));
      client.print(
          F("<span class='console-time'>--:--:--</span>System ready</div>"));
      client.print(F("</div></div>")); // End Console section

      client.print(F("</div>")); // End column 1

      // Column 2: Color Thresholds & Font Selection
      client.print(F("<div class='grid-column'>"));
      client.print(F("<div class='section'><h2>⏱️ Color Thresholds</h2>"));
      client.print(
          F("<p style='font-size:13px;color:#666;margin-bottom:20px'>"));
      client.print(
          F("The timer automatically changes color as time runs out</p>"));
      client.print(F("<div id='thresholds' class='threshold-list'></div>"));
      client.print(F("<button class='btn-add' onclick='addThreshold()'>+ Add "
                     "Threshold</button>"));
      client.print(F("<p style='font-size:13px;color:#666;margin:15px 0 10px "
                     "0;font-style:italic'>"));
      client.print(F("When no threshold matches:</p>"));
      client.print(F("<div class='threshold-default'>"));
      client.print(F("<span class='label'>Default Color</span>"));
      client.print(F("<span class='arrow'>→</span>"));
      client.print(F("<input type='color' id='defaultColor' value='#00FF00'>"));
      client.print(F("</div></div>"));

      client.print(F("<div class='section'><h2>🔤 Font Selection</h2>"));
      client.print(F("<div class='duration-card'>"));
      client.print(F("<label for='fontSelect' "
                     "style='margin-bottom:10px'>Display Font:</label>"));
      client.print(F("<select id='fontSelect' style='font-size:16px'>"));
      client.print(
          F("<option value='0'>Adafruit Default (5x7 @ 2x scale)</option>"));
      client.print(F("<optgroup label='Sans-Serif'>"));
      client.print(F("<option value='1'>Sans 9pt</option>"));
      client.print(F("<option value='2'>Sans 12pt</option>"));
      client.print(F("<option value='3'>Sans Bold 9pt</option>"));
      client.print(
          F("<option value='4' selected>Sans Bold 12pt (default)</option>"));
      client.print(F("</optgroup>"));
      client.print(F("<optgroup label='Monospace'>"));
      client.print(F("<option value='5'>Mono 9pt</option>"));
      client.print(F("<option value='6'>Mono 12pt</option>"));
      client.print(F("<option value='7'>Mono Bold 9pt</option>"));
      client.print(F("<option value='8'>Mono Bold 12pt</option>"));
      client.print(F("</optgroup>"));
      client.print(F("<optgroup label='Serif'>"));
      client.print(F("<option value='9'>Serif 9pt</option>"));
      client.print(F("<option value='10'>Serif 12pt</option>"));
      client.print(F("<option value='11'>Serif Bold 9pt</option>"));
      client.print(F("<option value='12'>Serif Bold 12pt</option>"));
      client.print(F("</optgroup>"));
      client.print(F("<optgroup label='Retro/Pixel'>"));
      client.print(F("<option value='13'>Org_01 (Retro @ 3x)</option>"));
      client.print(F("<option value='14'>Picopixel (Tiny @ 3x)</option>"));
      client.print(F("<option value='15'>TomThumb (Pixel @ 3x)</option>"));
      client.print(F("</optgroup>"));
      client.print(F("<optgroup label='Custom Fonts'>"));
      client.print(F("<option value='16'>Aquire (12pt)</option>"));
      client.print(F("<option value='17'>Aquire Bold (12pt)</option>"));
      client.print(F("<option value='18'>Aquire Light (12pt)</option>"));
      client.print(F("</optgroup>"));
      client.print(F("</select>"));
      client.print(F("<label for='letterSpacing' "
                     "style='margin-top:15px;margin-bottom:5px'>Character "
                     "Spacing:</label>"));
      client.print(F("<div style='display:flex;align-items:center;gap:10px'>"));
      client.print(F("<input type='range' id='letterSpacing' min='-2' max='5' "
                     "value='3' style='flex:1'>"));
      client.print(F("<span id='spacingValue' "
                     "style='min-width:30px;text-align:center'>3</span>"));
      client.print(F("</div>"));
      client.print(F("<label for='brightness' "
                     "style='margin-top:15px;margin-bottom:5px'>Display "
                     "Brightness:</label>"));
      client.print(F("<div style='display:flex;align-items:center;gap:10px'>"));
      client.print(F("<input type='range' id='brightness' min='0' max='255' "
                     "value='255' style='flex:1'>"));
      client.print(F("<span id='brightnessValue' "
                     "style='min-width:30px;text-align:center'>100%</span>"));
      client.print(
          F("</div></div></div></div>")); // End duration-card, Font Selection
                                          // section, and column 2

      // Column 3: System Status & WebSocket Connection
      client.print(F("<div class='grid-column'>"));

      // System Status Card
      client.print(F("<div class='section'><h2>📊 System Status</h2>"));
      client.print(F("<div class='info-display'>"));
      client.print(F("<div class='form-group'>IP Address</div>"));
      client.print(
          F("<div class='info-value' id='ipAddress'>Loading...</div>"));
      client.print(F("</div>"));
      client.print(F("<div class='info-display'>"));
      client.print(F("<div class='form-group'>FightTimer Connection</div>"));
      client.print(F("<div class='info-value' id='wsStatus'>"));
      client.print(F("<span style='color:#888'>Checking...</span></div>"));
      client.print(F("</div></div>")); // End System Status section

      // WebSocket Connection Card
      client.print(F("<div class='section'><h2>🔗 WebSocket Connection</h2>"));
      client.print(
          F("<div class='form-group'><label>Server Host / IP:</label>"));
      if (wsClient && wsClient->getHost().length() > 0) {
        client.print(F("<input type='text' id='wsHost' value='"));
        client.print(wsClient->getHost());
        client.print(F("'>"));
      } else {
        client.print(
            F("<input type='text' id='wsHost' value='172.17.17.156'>"));
      }

      client.print(F("</div><div class='form-group'><label>Port:</label>"));
      if (wsClient && wsClient->getPort() > 0) {
        client.print(F("<input type='number' id='wsPort' value='"));
        client.print(wsClient->getPort());
        client.print(F("' min='1' max='65535'>"));
      } else {
        client.print(F("<input type='number' id='wsPort' value='8766' min='1' "
                       "max='65535'>"));
      }

      client.print(F("</div><div class='form-group'><label>Path:</label>"));
      if (wsClient && wsClient->getPath().length() > 0) {
        client.print(F("<input type='text' id='wsPath' value='"));
        client.print(wsClient->getPath());
        client.print(F("'>"));
      } else {
        client.print(F("<input type='text' id='wsPath' value='/socket.io/'>"));
      }
      client.print(F("</div><div style='display:flex;gap:10px'>"));
      client.print(F("<button class='btn-start' onclick='connectWebSocket()' "
                     "style='flex:1'>"));
      client.print(F("🔗 Connect</button>"));
      client.print(F("<button class='btn-reset' "
                     "onclick='disconnectWebSocket()' style='flex:1'>"));
      client.print(
          F("❌ Disconnect</button></div></div>")); // End WebSocket Connection
                                                    // section

      client.print(F("</div>")); // End column 3

      client.print(F("</div>")); // End grid-container

      client.print(F("<button id='applyButton' class='btn-start apply-button' "
                     "onclick='applySettings()'>"));
      client.print(F("✓ Apply All Settings</button>"));

      client.print(F("</div>")); // End container
      client.print(F("<script>"));
      client.print(F("let thresholds=[];"));
      client.print(F("let consoleMessages=[];"));
      client.print(F("function addConsoleMessage(message,type='info'){"));
      client.print(F("const now=new Date();"));
      client.print(
          F("const time=now.toLocaleTimeString('en-US',{hour12:false});"));
      client.print(
          F("consoleMessages.push({time:time,message:message,type:type});"));
      client.print(F("if(consoleMessages.length>50)consoleMessages.shift();"));
      client.print(F("const console=document.getElementById('console');"));
      client.print(F("console.innerHTML='';"));
      client.print(F("consoleMessages.forEach(m=>{"));
      client.print(F("const entry=document.createElement('div');"));
      client.print(F("entry.className='console-entry console-'+m.type;"));
      client.print(F("entry.innerHTML='<span "
                     "class=\"console-time\">'+m.time+'</span>'+m.message;"));
      client.print(F("console.appendChild(entry);});"));
      client.print(F("console.scrollTop=console.scrollHeight;}"));
      client.print(F("function updateButtonState(){"));
      client.print(F("fetch('/api/status').then(r=>r.json()).then(data=>{"));
      client.print(F("const btn=document.getElementById('startBtn');"));
      client.print(F("if(data.isPaused){btn.textContent='▶️ Resume';}"));
      client.print(F("else{btn.textContent='▶️ Start';}"));
      client.print(F("}).catch(err=>console.log('Status check failed'));}"));
      client.print(F("function loadSettings(){"));
      client.print(F("fetch('/api/settings').then(r=>r.json()).then(data=>{"));
      client.print(F("if(data.duration){"));
      client.print(F("document.getElementById('durationMin').value=Math.floor("
                     "data.duration/60);"));
      client.print(
          F("document.getElementById('durationSec').value=data.duration%60;"));
      client.print(F("}"));
      client.print(F("if(data.fontId!==undefined){"));
      client.print(
          F("document.getElementById('fontSelect').value=data.fontId;"));
      client.print(F("}"));
      client.print(F("if(data.spacing!==undefined){"));
      client.print(
          F("document.getElementById('letterSpacing').value=data.spacing;"));
      client.print(F(
          "document.getElementById('spacingValue').textContent=data.spacing;"));
      client.print(F("}"));
      client.print(F("if(data.brightness!==undefined){"));
      client.print(
          F("document.getElementById('brightness').value=data.brightness;"));
      client.print(F("const percent=Math.round((data.brightness/255)*100);"));
      client.print(F("document.getElementById('brightnessValue').textContent="
                     "percent+'%';"));
      client.print(F("}"));
      client.print(F("}).catch(err=>console.log('Load settings failed'));}"));
      client.print(F("function loadThresholds(){"));
      client.print(
          F("fetch('/api/thresholds').then(r=>r.json()).then(data=>{"));
      client.print(F("thresholds=data.thresholds||[];"));
      client.print(F("if(data.defaultColor){document.getElementById('"
                     "defaultColor').value=data.defaultColor;}"));
      client.print(F("renderThresholds();"));
      client.print(F("}).catch(err=>console.log('Load failed'));}"));
      client.print(F("function renderThresholds(){"));
      client.print(F("const container=document.getElementById('thresholds');"));
      client.print(F("container.innerHTML='';"));
      client.print(F("thresholds.forEach((t,i)=>{"));
      client.print(F("const div=document.createElement('div');"));
      client.print(F("div.className='threshold-item';"));
      client.print(
          F("const mins=Math.floor(t.seconds/60);const secs=t.seconds%60;"));
      client.print(F("div.innerHTML=`<div class='time-inputs'>"));
      client.print(F("<span class='when-label'>When ≤</span>"));
      client.print(F("<input type='number' value='${mins}' min='0' max='60' "));
      client.print(
          F("onchange='updateThreshold(${i},\"minutes\",this.value)'>"));
      client.print(F("<span class='time-label'>min</span>"));
      client.print(F("<input type='number' value='${secs}' min='0' max='59' "));
      client.print(
          F("onchange='updateThreshold(${i},\"seconds\",this.value)'>"));
      client.print(F("<span class='time-label'>sec</span></div>"));
      client.print(F("<span class='arrow'>→</span>"));
      client.print(F("<input type='color' value='${t.color}' "));
      client.print(F("onchange='updateThreshold(${i},\"color\",this.value)'>"));
      client.print(F("<button class='btn-remove' "
                     "onclick='removeThreshold(${i})'>✕</button>`;"));
      client.print(F("container.appendChild(div);});}"));
      client.print(F("function addThreshold(){"));
      client.print(F("thresholds.push({seconds:60,color:'#FFFF00'});"
                     "renderThresholds();}"));
      client.print(
          F("function "
            "removeThreshold(i){thresholds.splice(i,1);renderThresholds();}"));
      client.print(F("function updateThreshold(i,field,value){"));
      client.print(
          F("if(field==='minutes'){const s=thresholds[i].seconds%60;"));
      client.print(F("thresholds[i].seconds=parseInt(value)*60+s;}"));
      client.print(F("else if(field==='seconds'){const "
                     "m=Math.floor(thresholds[i].seconds/60);"));
      client.print(F("thresholds[i].seconds=m*60+parseInt(value);}"));
      client.print(F("else if(field==='color'){thresholds[i].color=value;}}"));
      client.print(F("function sendCommand(cmd){"));
      client.print(
          F("fetch('/api',{method:'POST',headers:{'Content-Type':'application/"
            "x-www-form-urlencoded'},"));
      client.print(F("body:'action='+cmd}).then(r=>r.text()).then(data=>{"));
      client.print(F("addConsoleMessage('Command: "
                     "'+cmd,data.includes('Error')?'error':'success');"
                     "updateButtonState();})"));
      client.print(F(".catch(()=>addConsoleMessage('Error sending command: "
                     "'+cmd,'error'))}"));
      client.print(F("function toggleOrientation(){"));
      client.print(
          F("fetch('/api',{method:'POST',headers:{'Content-Type':'application/"
            "x-www-form-urlencoded'},"));
      client.print(F("body:'action=flip'}).then(r=>r.text()).then(data=>{"));
      client.print(F("addConsoleMessage('Display "
                     "flipped',data.includes('Error')?'error':'success');})"));
      client.print(F(
          ".catch(()=>addConsoleMessage('Error flipping display','error'))}"));
      client.print(F("function applySettings(){"));
      client.print(F("const "
                     "durationMin=parseInt(document.getElementById('"
                     "durationMin').value)||0;"));
      client.print(F("const "
                     "durationSec=parseInt(document.getElementById('"
                     "durationSec').value)||0;"));
      client.print(F("const duration=durationMin*60+durationSec;"));
      client.print(F(
          "const defaultColor=document.getElementById('defaultColor').value;"));
      client.print(
          F("const font=document.getElementById('fontSelect').value;"));
      client.print(
          F("const spacing=document.getElementById('letterSpacing').value;"));
      client.print(
          F("const brightness=document.getElementById('brightness').value;"));
      client.print(F("const "
                     "thresholdData=thresholds.map(t=>t.seconds+':'+t.color)."
                     "join('|');"));
      client.print(F("let params='action=settings&duration='+duration"
                     "+'&font='+font+'&spacing='+spacing+'&brightness='+"
                     "brightness+'&thresholds='+"
                     "encodeURIComponent(thresholdData)+'&default='+"
                     "encodeURIComponent(defaultColor);"));
      client.print(
          F("fetch('/api/"
            "settings',{method:'POST',headers:{'Content-Type':'application/"
            "x-www-form-urlencoded'},"));
      client.print(F("body:params}).then(r=>r.text()).then(data=>{"
                     "addConsoleMessage('Settings saved successfully',"
                     "'success');}).catch(()=>addConsoleMessage('Error "
                     "saving settings','error'))}"));
      client.print(F("document.getElementById('letterSpacing')."
                     "addEventListener('input',function(){"));
      client.print(F("document.getElementById('spacingValue').textContent=this."
                     "value;});"));
      client.print(F("document.getElementById('brightness').addEventListener('"
                     "input',function(){"));
      client.print(F("const percent=Math.round((this.value/255)*100);"));
      client.print(F("document.getElementById('brightnessValue').textContent="
                     "percent+'%';});"));

      // Network and WebSocket status functions
      client.print(F("function updateNetworkStatus(){"));
      client.print(
          F("fetch('/api/network/status').then(r=>r.json()).then(data=>{"));
      client.print(
          F("document.getElementById('ipAddress').textContent=data.ip;"));
      client.print(F("}).catch(()=>{document.getElementById('ipAddress')."
                     "textContent='Error';});}"));

      client.print(F("function updateWebSocketStatus(){"));
      client.print(
          F("fetch('/api/websocket/status').then(r=>r.json()).then(data=>{"));
      client.print(F("const wsStatus=document.getElementById('wsStatus');"));
      client.print(F("if(data.connected){"));
      client.print(F("wsStatus.innerHTML='<span style=\"color:#4CAF50\">✅ "
                     "Connected to '+data.url+'</span>';}"));
      client.print(F("else{wsStatus.innerHTML='<span style=\"color:#888\">⚪ "
                     "Not connected</span>';}"));
      client.print(F("}).catch(()=>{});}"));

      client.print(F("function connectWebSocket(){"));
      client.print(F("const host=document.getElementById('wsHost').value;"));
      client.print(F("const port=document.getElementById('wsPort').value;"));
      client.print(F("const path=document.getElementById('wsPath').value;"));
      client.print(F("if(!host){addConsoleMessage('Please enter a "
                     "host','error');return;}"));
      client.print(F("const params=new "
                     "URLSearchParams({host:host,port:port,path:path});"));
      client.print(
          F("fetch('/api/websocket/connect',{method:'POST',body:params})"));
      client.print(F(".then(r=>r.json()).then(data=>{"));
      client.print(F("addConsoleMessage(data.message,data.status==='success'?'"
                     "success':'error');"));
      client.print(F("setTimeout(updateWebSocketStatus,1000);"));
      client.print(
          F("}).catch(()=>addConsoleMessage('Connection failed','error'));}"));

      client.print(F("function disconnectWebSocket(){"));
      client.print(F("fetch('/api/websocket/disconnect',{method:'POST'})"));
      client.print(F(".then(r=>r.json()).then(data=>{"));
      client.print(F("addConsoleMessage(data.message,data.status==='success'?'"
                     "success':'error');"));
      client.print(F("setTimeout(updateWebSocketStatus,1000);"));
      client.print(
          F("}).catch(()=>addConsoleMessage('Disconnect failed','error'));}"));

      // Sticky button logic
      client.print(F("function updateStickyButton(){"));
      client.print(F("const button=document.getElementById('applyButton');"));
      client.print(F("const container=document.querySelector('.container');"));
      client.print(
          F("container.classList.remove('content-with-sticky');")); // Remove
                                                                    // first to
                                                                    // get true
                                                                    // height
      client.print(
          F("const scrollDiff=document.body.scrollHeight-window.innerHeight;"));
      client.print(F("const needsScroll=scrollDiff>100;")); // Only sticky if
                                                            // >100px overflow
      client.print(F("if(needsScroll){"));
      client.print(F("button.classList.add('sticky');"));
      client.print(F("container.classList.add('content-with-sticky');}"));
      client.print(F("else{"));
      client.print(F("button.classList.remove('sticky');"));
      client.print(F("container.classList.remove('content-with-sticky');}}"));
      client.print(F("window.addEventListener('resize',updateStickyButton);"));

      client.print(F("loadSettings();"));
      client.print(F("loadThresholds();"));
      client.print(F("updateButtonState();"));
      client.print(F("updateNetworkStatus();"));
      client.print(F("updateWebSocketStatus();"));
      client.print(F("updateStickyButton();"));
      client.print(F("setInterval(updateButtonState, 1000);"));
      client.print(F("setInterval(updateNetworkStatus, 5000);"));
      client.print(F("setInterval(updateWebSocketStatus, 5000);"));
      client.print(F("setInterval(updateStickyButton, 500);"));
      // Overwrite updateWebSocketStatus with logging version
      client.print(F("let lastWsState=false;"));
      client.print(F("updateWebSocketStatus=function(){"));
      client.print(
          F("fetch('/api/websocket/status').then(r=>r.json()).then(data=>{"));
      client.print(F("const wsStatus=document.getElementById('wsStatus');"));
      client.print(F("if(data.connected){"));
      client.print(F("wsStatus.innerHTML='<span style=\"color:#4CAF50\">✅ "
                     "Connected to '+data.url+'</span>';"));
      client.print(F("if(!lastWsState){addConsoleMessage('WebSocket Connected "
                     "to '+data.url, 'success');}"));
      client.print(F("lastWsState=true;"));
      client.print(F("}else{"));
      client.print(F("wsStatus.innerHTML='<span style=\"color:#888\">⚪ "
                     "Not connected</span>';"));
      client.print(F("if(lastWsState){addConsoleMessage('WebSocket "
                     "Disconnected', 'warning');}"));
      client.print(F("lastWsState=false;"));
      client.print(F("}"));
      client.print(F("}).catch(()=>{});};"));

      client.print(F("</script></body></html>"));

      client.stop();
      DEBUG_PRINTLN("Web page served");
    } else if (requestPath.startsWith("/api")) {
      DEBUG_PRINT("API Request: ");
      DEBUG_PRINT(requestType);
      DEBUG_PRINT(" ");
      DEBUG_PRINTLN(requestPath);

      if (requestType == "POST") {
        if (requestPath == "/api") {
          // Timer Control
          String action = "";
          int equalIndex = postData.indexOf('=');
          if (equalIndex > 0 && postData.substring(0, equalIndex) == "action") {
            action = postData.substring(equalIndex + 1);
          }

          DEBUG_PRINT("Timer Action: ");
          DEBUG_PRINTLN(action);

          String response = "{";
          if (action == "start") {
            timerDisplay.getTimer().start();
            response += "\"status\":\"success\",\"message\":\"Timer started\"";
          } else if (action == "pause") {
            timerDisplay.getTimer().stop();
            response += "\"status\":\"success\",\"message\":\"Timer paused\"";
          } else if (action == "reset") {
            timerDisplay.getTimer().reset();
            response += "\"status\":\"success\",\"message\":\"Timer reset\"";
          } else if (action == "flip") {
            current_orientation = (current_orientation == 0) ? 180 : 0;
            // RGBMatrix::setOrientation(current_orientation);
            response +=
                "\"status\":\"success\",\"message\":\"Orientation flipped\"";
          } else {
            response +=
                "\"status\":\"error\",\"message\":\"Unknown action: " + action +
                "\"";
          }
          response += "}";
          sendHTTPResponse(client, 200, "application/json", response);

        } else if (requestPath == "/api/settings") {
          // Parse all settings from consolidated POST
          int duration = 180;
          int fontId = 0;
          int spacing = 3;
          int brightness = 255;
          String thresholdsData = "";
          String defaultColorData = "";

          // Robust key-value parsing
          int pos = 0;
          while (pos < postData.length()) {
            int amp = postData.indexOf('&', pos);
            if (amp == -1)
              amp = postData.length();
            String pair = postData.substring(pos, amp);
            int eq = pair.indexOf('=');
            if (eq > 0) {
              String key = pair.substring(0, eq);
              String val = urlDecode(pair.substring(eq + 1));
              if (key == "duration")
                duration = val.toInt();
              else if (key == "font")
                fontId = val.toInt();
              else if (key == "spacing")
                spacing = val.toInt();
              else if (key == "brightness")
                brightness = val.toInt();
              else if (key == "thresholds")
                thresholdsData = val;
              else if (key == "default")
                defaultColorData = val;
            }
            pos = amp + 1;
          }

          // Apply Timer Duration
          Timer::Components comp;
          comp.minutes = duration / 60;
          comp.seconds = duration % 60;
          comp.milliseconds = 0;
          timerDisplay.getTimer().setDuration(comp);
          timerDisplay.getTimer().reset();

          // Apply Display Settings
          timerDisplay.setFont(getFontById(fontId), fontId); // Fix: Pass fontId
          timerDisplay.setTextSize(getTextSizeForFont(fontId));
          timerDisplay.setLetterSpacing(spacing);
          timerDisplay.setBrightness(brightness);

          // Apply Color Thresholds
          if (thresholdsData.length() > 0) {
            timerDisplay.clearColorThresholds();
            int start = 0;
            while (start < thresholdsData.length()) {
              int end = thresholdsData.indexOf('|', start);
              if (end == -1)
                end = thresholdsData.length();

              String token = thresholdsData.substring(start, end);
              int colon = token.indexOf(':');
              if (colon > 0) {
                int seconds = token.substring(0, colon).toInt();
                String color = token.substring(colon + 1);
                uint8_t r, g, b;
                parseColor(color, r, g, b);
                timerDisplay.addColorThreshold(seconds, r, g, b);
              }
              start = end + 1;
            }
          }

          // Apply Default Color
          if (defaultColorData.length() > 0) {
            uint8_t r, g, b;
            parseColor(defaultColorData, r, g, b);
            timerDisplay.setDefaultColor(r, g, b);
            // Also set current color to default if timer is idle
            if (timerDisplay.getTimer().isIdle()) {
              timerDisplay.setColor(r, g, b);
            }
          }

          // Persist all changes
          if (saveSettings(timerDisplay)) {
            sendHTTPResponse(client, 200, "text/plain", "Settings saved");
          } else {
            sendHTTPResponse(client, 500, "text/plain",
                             "Error saving settings");
          }
        } else if (requestPath == "/api/websocket/connect") {
          // WebSocket Connect
          String host = "";
          int port = 8765;
          String path = "/socket.io/";

          // Simple parser for urlencoded body
          int pos = 0;
          while (pos < postData.length()) {
            int amp = postData.indexOf('&', pos);
            if (amp == -1)
              amp = postData.length();
            String pair = postData.substring(pos, amp);
            int eq = pair.indexOf('=');
            if (eq > 0) {
              String key = pair.substring(0, eq);
              String val = urlDecode(pair.substring(eq + 1));
              if (key == "host")
                host = val;
              else if (key == "port")
                port = val.toInt();
              else if (key == "path")
                path = val;
            }
            pos = amp + 1;
          }

          if (wsClient && host.length() > 0) {
            wsClient->connect(host.c_str(), port, path.c_str());
            sendHTTPResponse(
                client, 200, "application/json",
                "{\"status\":\"success\",\"message\":\"Connecting...\"}");
          } else {
            sendHTTPResponse(
                client, 400, "application/json",
                "{\"status\":\"error\",\"message\":\"Missing host\"}");
          }
        } else if (requestPath == "/api/websocket/disconnect") {
          if (wsClient) {
            wsClient->disconnect();
            sendHTTPResponse(
                client, 200, "application/json",
                "{\"status\":\"success\",\"message\":\"Disconnected\"}");
          } else {
            sendHTTPResponse(
                client, 500, "application/json",
                "{\"status\":\"error\",\"message\":\"No Client\"}");
          }
        } else {
          sendHTTPResponse(client, 404, "text/plain", "Not Found");
        }
      } else {
        // GET Requests
        if (requestPath == "/api/status") {
          String json = "{";
          json += "\"isPaused\":" +
                  String(timerDisplay.getTimer().isPaused() ? "true" : "false");
          json += "}";
          sendHTTPResponse(client, 200, "application/json", json);
        } else if (requestPath == "/api/thresholds") {
          JsonDocument doc;
          JsonArray thresholds = doc["thresholds"].to<JsonArray>();
          size_t count = 0;
          const TimerDisplay::ColorThreshold *data =
              timerDisplay.getColorThresholds(count);
          for (size_t i = 0; i < count; i++) {
            JsonObject t = thresholds.add<JsonObject>();
            t["seconds"] = data[i].seconds;
            char colorHex[8];
            snprintf(colorHex, sizeof(colorHex), "#%02X%02X%02X", data[i].r,
                     data[i].g, data[i].b);
            t["color"] = colorHex;
          }

          uint8_t dr, dg, db;
          timerDisplay.getDefaultColor(dr, dg, db);
          char defaultHex[8];
          snprintf(defaultHex, sizeof(defaultHex), "#%02X%02X%02X", dr, dg, db);
          doc["defaultColor"] = defaultHex;

          String response;
          serializeJson(doc, response);
          sendHTTPResponse(client, 200, "application/json", response);
        } else if (requestPath == "/api/settings") {
          JsonDocument doc;
          doc["fontId"] = timerDisplay.getFontId();
          doc["spacing"] = timerDisplay.getLetterSpacing();
          doc["brightness"] = timerDisplay.getBrightness();
          doc["duration"] = timerDisplay.getTimer().getDurationSeconds();

          String response;
          serializeJson(doc, response);
          sendHTTPResponse(client, 200, "application/json", response);
        } else if (requestPath == "/api/network/status") {
          String json = "{\"ip\":\"" + getIPAddressString() + "\"}";
          sendHTTPResponse(client, 200, "application/json", json);
        } else if (requestPath == "/api/websocket/status") {
          String json = "{";
          if (wsClient) {
            bool connected = wsClient->isConnected();
            Serial.print("API Status: ");
            Serial.println(connected ? "Connected" : "Not Connected");
            json +=
                "\"connected\":" + String(connected ? "true" : "false") + ",";
            json += "\"url\":\"" + String(wsClient->getServerUrl()) + "\"";
          } else {
            json += "\"connected\":false";
          }
          json += "}";
          sendHTTPResponse(client, 200, "application/json", json);
        } else {
          sendHTTPResponse(client, 404, "text/plain", "Not Found");
        }
      }
      client.stop();
      DEBUG_PRINTLN("API request handled");
    } else {
      sendHTTPResponse(client, 404, "text/plain", "Not Found");
      client.stop();
    }
  }
}

} // namespace WebServer
