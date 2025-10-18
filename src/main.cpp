#include <Arduino.h>
#include <RGBMatrix.h>
#include <TimerDisplay.h>
#include <WebServer.h>
#include <WebSocketClient.h>

// Debug flag - set to false to disable debug messages for better timing
#define DEBUG_MAIN false

// Debug printing macros
#if DEBUG_MAIN
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif

// Include various fonts (12pt and below for 64x32 display)
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
// Retro/pixel fonts
#include <Fonts/Org_01.h>
#include <Fonts/Picopixel.h>
#include <Fonts/TomThumb.h>
// Custom fonts
#include <CustomFonts/Aquire_BW0ox12pt7b.h>
#include <CustomFonts/AquireBold_8Ma6012pt7b.h>
#include <CustomFonts/AquireLight_YzE0o12pt7b.h>

// Network configuration
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
uint8_t static_ip[] = {10, 0, 0, 21};  // Static IP address
const char* hostname = "arenatimer";     // Access via http://arenatimer.local

// Create timer display in TIMER mode (countdown)
TimerDisplay timerDisplay(RGBMatrix::getMatrix(), TimerDisplay::Mode::TIMER);

// Create WebSocket client for FightTimer integration
WebSocketClient* wsClient = nullptr;

void setup()
{
  Serial.begin(115200);
  delay(1000);
  
  DEBUG_PRINTLN("\n=== Arena Timer Firmware ===");
  
  // Initialize RGB Matrix
  DEBUG_PRINTLN("Initializing RGB Matrix...");
  RGBMatrix::init();
  RGBMatrix::setOrientation(180);
  RGBMatrix::clear();
  
  // Initialize Ethernet with static IP
  DEBUG_PRINTLN("Initializing Ethernet...");
  bool ethernet_ok = false;
  String ip_address;
  
  if (WebServer::init(mac, static_ip))
  {
    ethernet_ok = true;
    ip_address = WebServer::getIPAddressString();
    DEBUG_PRINT("IP Address: ");
    DEBUG_PRINTLN(ip_address);
    
    // Initialize mDNS for easy hostname access
    if (WebServer::initMDNS(hostname))
    {
      DEBUG_PRINT("Access timer at: http://");
      DEBUG_PRINT(hostname);
      DEBUG_PRINTLN(".local");
    }
  }
  else
  {
    DEBUG_PRINTLN("ERROR: Ethernet initialization failed!");
    DEBUG_PRINTLN("Timer will work, but web interface is unavailable.");
    DEBUG_PRINTLN("Check your W5500 wiring and connections.");
  }
  
  if (ethernet_ok)
  {
    DEBUG_PRINT("IP Address: ");
    DEBUG_PRINTLN(ip_address);
    
    // Display IP address on LED matrix for 5 seconds
    RGBMatrix::clear();
    RGBMatrix::getMatrix().setFont(nullptr);  // Use default small font
    RGBMatrix::getMatrix().setTextSize(1);
    RGBMatrix::getMatrix().setTextColor(RGBMatrix::getMatrix().color565(0, 255, 0));  // Green
    
    // Split IP into two lines if too long
    if (ip_address.length() > 15)
    {
      RGBMatrix::getMatrix().setCursor(2, 6);
      RGBMatrix::getMatrix().print("IP:");
      RGBMatrix::getMatrix().setCursor(2, 16);
      RGBMatrix::getMatrix().print(ip_address.substring(0, 10));
      RGBMatrix::getMatrix().setCursor(2, 26);
      RGBMatrix::getMatrix().print(ip_address.substring(10));
    }
    else
    {
      RGBMatrix::getMatrix().setCursor(2, 10);
      RGBMatrix::getMatrix().print("IP:");
      RGBMatrix::getMatrix().setCursor(2, 22);
      RGBMatrix::getMatrix().print(ip_address);
    }
    
    RGBMatrix::show();
    delay(5000);  // Show IP for 5 seconds
    
    // Start web server
    WebServer::startWebServer(80);
    DEBUG_PRINTLN("Web server started!");
  }
  
  // Configure initial timer display
  DEBUG_PRINTLN("Configuring timer display...");
  timerDisplay.setFont(&FreeSansBold12pt7b);
  // Default color thresholds already set in constructor: Blue (default), Yellow (<2min), Red (<1min)
  timerDisplay.getTimer().setDuration({3, 0, 0});  // 3 minutes default
  timerDisplay.getTimer().reset();
  
  // Initialize WebSocket client
  DEBUG_PRINTLN("Initializing WebSocket client...");
  wsClient = new WebSocketClient(&timerDisplay.getTimer());
  DEBUG_PRINTLN("WebSocket client ready (not connected)");
  
  // Pass WebSocket client to WebServer for API access
  WebServer::setWebSocketClient(wsClient);
  
  DEBUG_PRINTLN("\n=== Setup Complete ===");
  if (ethernet_ok)
  {
    DEBUG_PRINTLN("Web Interface:");
    DEBUG_PRINT("  - http://");
    DEBUG_PRINT(hostname);
    DEBUG_PRINTLN(".local");
    DEBUG_PRINT("  - http://");
    DEBUG_PRINTLN(ip_address);
  }
  DEBUG_PRINTLN("======================\n");
}

void loop()
{
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
  if (millis() - last_serial >= 5000)
  {
    Timer::Components remaining = timerDisplay.getTimer().getRemainingTime();
    DEBUG_PRINT("Status: ");
    if (timerDisplay.getTimer().isExpired())
    {
      DEBUG_PRINT("EXPIRED ");
    }
    else if (timerDisplay.getTimer().isRunning())
    {
      DEBUG_PRINT("RUNNING ");
    }
    else if (timerDisplay.getTimer().isPaused())
    {
      DEBUG_PRINT("PAUSED ");
    }
    else if (timerDisplay.getTimer().isIdle())
    {
      DEBUG_PRINT("IDLE ");
    }
    
    DEBUG_PRINT("| Time: ");
    DEBUG_PRINT(remaining.minutes);
    DEBUG_PRINT(":");
    if (remaining.seconds < 10) DEBUG_PRINT("0");
    DEBUG_PRINT(remaining.seconds);
    DEBUG_PRINT(".");
    if (remaining.milliseconds < 100) DEBUG_PRINT("0");
    if (remaining.milliseconds < 10) DEBUG_PRINT("0");
    DEBUG_PRINTLN(remaining.milliseconds);
    
    last_serial = millis();
  }
}
