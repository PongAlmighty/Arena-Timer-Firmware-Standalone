/**
 * Web Server - HTTP server, web interface, and API for Arena Timer
 * Handles web UI, RESTful API endpoints, and network communications
 */

#pragma once

#include <Ethernet.h>
#include <TimerDisplay.h>

#include "ConfigManager.h"

// Forward declaration
class WebSocketClient;

namespace WebServer {
// Pin definitions for W5500
extern const int CS;
extern const int SCK_PIN;
extern const int MOSI_PIN;
extern const int MISO_PIN;

/// @brief Initialize the Ethernet connection with config
/// @param mac MAC address (6 bytes)
/// @param config Network configuration
/// @return true if successful, false otherwise
// Initialize with static IP (matching backup source)
bool init(uint8_t mac[6], uint8_t ip[4]);

/// @brief Initialize mDNS responder for hostname resolution
/// @param hostname Hostname (without .local suffix)
/// @return true if successful, false otherwise
bool initMDNS(const char *hostname);

/// @brief Update mDNS responder (call in loop)
void updateMDNS();

/// @brief Start the web server on specified port
/// @param port Port number (default 80)
void startWebServer(uint16_t port = 80);

/// @brief Handle incoming client connections (call in loop)
/// @param timerDisplay Reference to the TimerDisplay object to control
void handleClient(TimerDisplay &timerDisplay);

/// @brief Set the WebSocket client instance for API access
/// @param wsClient Pointer to WebSocketClient instance
void setWebSocketClient(WebSocketClient *wsClient);

/// @brief Get the current Ethernet server
EthernetServer &getServer();

/// @brief Get the IP address as a string
/// @return IP address string (e.g., "192.168.1.100")
String getIPAddressString();
} // namespace WebServer
