#include <Arduino.h>
#include <RGBMatrix.h>
#include <TimerDisplay.h>
#include <Comms.h>

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
#include <Aquire_BW0ox12pt7b.h>
#include <AquireBold_8Ma6012pt7b.h>
#include <AquireLight_YzE0o12pt7b.h>

// Network configuration// Network configuration
uint8_t mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
uint8_t static_ip[] = {10, 0, 0, 21};  // Static IP address
const char* hostname = "arenatimer";     // Access via http://arenatimer.local

// Create timer display in TIMER mode (countdown)
TimerDisplay timerDisplay(RGBMatrix::getMatrix(), TimerDisplay::Mode::TIMER);

void setup()
{
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== Arena Timer Firmware ===");
  
  // Initialize RGB Matrix
  Serial.println("Initializing RGB Matrix...");
  RGBMatrix::init();
  RGBMatrix::setOrientation(180);
  RGBMatrix::clear();
  
  // Initialize Ethernet with static IP
  Serial.println("Initializing Ethernet...");
  bool ethernet_ok = false;
  String ip_address;
  
  if (Comms::init(mac, static_ip))
  {
    ethernet_ok = true;
    ip_address = Comms::getIPAddressString();
    Serial.print("IP Address: ");
    Serial.println(ip_address);
    
    // Initialize mDNS for easy hostname access
    if (Comms::initMDNS(hostname))
    {
      Serial.print("Access timer at: http://");
      Serial.print(hostname);
      Serial.println(".local");
    }
  }
  else
  {
    Serial.println("ERROR: Ethernet initialization failed!");
    Serial.println("Timer will work, but web interface is unavailable.");
    Serial.println("Check your W5500 wiring and connections.");
  }
  
  if (ethernet_ok)
  {
    Serial.print("IP Address: ");
    Serial.println(ip_address);
    
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
    Comms::startWebServer(80);
    Serial.println("Web server started!");
  }
  
  // Configure initial timer display
  Serial.println("Configuring timer display...");
  timerDisplay.setFont(&FreeSansBold12pt7b);
  // Default color thresholds already set in constructor: Blue (default), Yellow (<2min), Red (<1min)
  timerDisplay.getTimer().setDuration({3, 0, 0});  // 3 minutes default
  timerDisplay.getTimer().reset();
  
  Serial.println("\n=== Setup Complete ===");
  if (ethernet_ok)
  {
    Serial.println("Web Interface:");
    Serial.print("  - http://");
    Serial.print(hostname);
    Serial.println(".local");
    Serial.print("  - http://");
    Serial.println(ip_address);
  }
  Serial.println("======================\n");
}

void loop()
{
  // Handle incoming web requests (includes mDNS update)
  Comms::handleClient(timerDisplay);
  
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
    Serial.print("Status: ");
    if (timerDisplay.getTimer().isExpired())
    {
      Serial.print("EXPIRED ");
    }
    else if (timerDisplay.getTimer().isRunning())
    {
      Serial.print("RUNNING ");
    }
    else if (timerDisplay.getTimer().isPaused())
    {
      Serial.print("PAUSED ");
    }
    else if (timerDisplay.getTimer().isIdle())
    {
      Serial.print("IDLE ");
    }
    
    Serial.print("| Time: ");
    Serial.print(remaining.minutes);
    Serial.print(":");
    if (remaining.seconds < 10) Serial.print("0");
    Serial.print(remaining.seconds);
    Serial.print(".");
    if (remaining.milliseconds < 100) Serial.print("0");
    if (remaining.milliseconds < 10) Serial.print("0");
    Serial.println(remaining.milliseconds);
    
    last_serial = millis();
  }
  
  // Small delay to prevent overwhelming the system
  delay(10);
}
