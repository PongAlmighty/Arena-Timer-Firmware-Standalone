#include "WebSocketClient.h"

// Debug flag - ENABLED temporarily for debugging
#define DEBUG_WEBSOCKET true

// Debug printing macros
#if DEBUG_WEBSOCKET
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// Static instance pointer for callback
WebSocketClient *WebSocketClient::_instance = nullptr;

WebSocketClient::WebSocketClient(Timer *timer)
    : _timer(timer), _connected(false), _connectionAttempted(false),
      _manuallyDisconnected(false), _lastReconnectAttempt(0),
      _reconnectInterval(10000), _autoReconnect(true), _serverPort(8765),
      _connectInProgress(false), _consecutiveFailures(0) {

  // Load saved settings
  loadSettings();

  // Set instance for static callback
  _instance = this;

  // Set up event handler
  _client.onEvent(webSocketEvent);

  // Disable SSL verification (not needed for local connections)
  // Disable library auto-reconnect - we'll handle it manually with proper
  // backoff
  _client.setReconnectInterval(0);
}

bool WebSocketClient::connect(const char *host, uint16_t port,
                              const char *path) {
  if (_connected) {
    disconnect();
  }

  _connectionAttempted = true; // Mark that user has attempted connection
  _manuallyDisconnected =
      false;                  // Clear manual disconnect flag when reconnecting
  _connectInProgress = false; // Reset connection progress flag
  _consecutiveFailures = 0;   // Reset failure counter for new connection
  _serverHost = String(host);
  _serverPort = port;
  _serverPath = String(path);

  // Build URL for display
  _fullUrl = "ws://" + _serverHost + ":" + String(_serverPort) + _serverPath;

  DEBUG_PRINT("Connecting to server: ");
  DEBUG_PRINTLN(_fullUrl);

  // Check for Socket.IO request
  bool isSocketIO = (_serverPath.indexOf("/socket.io") >= 0);

  if (isSocketIO) {
    DEBUG_PRINTLN(
        "Detected Socket.IO path - trying direct WebSocket connection");
    DEBUG_PRINTLN("Socket.IO will be handled at message level");
    String socketIOPath = _serverPath + "?EIO=4&transport=websocket";
    DEBUG_PRINT("Using path: ");
    DEBUG_PRINTLN(socketIOPath);
    _client.begin(_serverHost.c_str(), _serverPort, socketIOPath.c_str());
  } else {
    DEBUG_PRINTLN("Using standard WebSocket connection");
    _client.begin(_serverHost.c_str(), _serverPort, _serverPath.c_str());
  }

  // CRITICAL: Set library's internal reconnect interval to 60 seconds
  // This prevents the library's loop() from retrying too fast when writes fail
  _client.setReconnectInterval(60000);

  // Connection result will come via callback
  Serial.println("Connection initiated with 60s retry interval...");

  // Save settings on successful initiation (user intent)
  saveSettings();

  return true; // Actual connection status will be updated via callback
}

void WebSocketClient::loadSettings() {
  EEPROM.begin(512);            // Initialize EEPROM
  if (EEPROM.read(0) == 0x42) { // Magic byte
    // Host
    int len = EEPROM.read(1);
    char buf[128];
    for (int i = 0; i < len && i < 127; i++) {
      buf[i] = EEPROM.read(2 + i);
    }
    buf[len] = 0;
    _serverHost = String(buf);

    // Port
    uint8_t low = EEPROM.read(120);
    uint8_t high = EEPROM.read(121);
    _serverPort = (high << 8) | low;

    // Path
    len = EEPROM.read(122);
    for (int i = 0; i < len && i < 127; i++) {
      buf[i] = EEPROM.read(123 + i);
    }
    buf[len] = 0;
    _serverPath = String(buf);

    Serial.println("Loaded saved WebSocket settings:");
    Serial.print("Host: ");
    Serial.println(_serverHost);
    Serial.print("Port: ");
    Serial.println(_serverPort);
    Serial.print("Path: ");
    Serial.println(_serverPath);

    // Update full URL
    _fullUrl = "ws://" + _serverHost + ":" + String(_serverPort) + _serverPath;

    // Auto-connect on boot
    _connectionAttempted = true;
  } else {
    Serial.println("No saved settings found in EEPROM");
  }
}

void WebSocketClient::saveSettings() {
  EEPROM.begin(512);
  EEPROM.write(0, 0x42); // Magic byte

  // Host
  int len = _serverHost.length();
  if (len > 100)
    len = 100; // Cap length
  EEPROM.write(1, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(2 + i, _serverHost[i]);
  }

  // Port
  EEPROM.write(120, _serverPort & 0xFF);
  EEPROM.write(121, (_serverPort >> 8) & 0xFF);

  // Path
  len = _serverPath.length();
  if (len > 100)
    len = 100; // Cap length
  EEPROM.write(122, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(123 + i, _serverPath[i]);
  }

  EEPROM.commit();
  Serial.println("Saved WebSocket settings to EEPROM");
}

void WebSocketClient::disconnect() {
  DEBUG_PRINTLN("Disconnect requested...");

  // Set flags first to prevent any race conditions
  _manuallyDisconnected = true; // Mark as manually disconnected
  _connectionAttempted = false; // Clear connection attempt flag
  _connected = false;           // Set disconnected state

  // Now disconnect from the WebSocket
  _client.disconnect();

  // Force stop any internal reconnection by reinitializing the client
  _client = WebSocketsClient();    // Reset the client
  _client.onEvent(webSocketEvent); // Reattach event handler
  _client.setReconnectInterval(0); // Disable auto-reconnect

  DEBUG_PRINTLN("WebSocket forcibly disconnected and reset");
  DEBUG_PRINT("Manual disconnect flag set: ");
  DEBUG_PRINTLN(_manuallyDisconnected ? "true" : "false");
}

bool WebSocketClient::isConnected() { return _connected; }

void WebSocketClient::poll() {
  // Only poll if we've actually attempted a connection
  // Otherwise the library fires continuous disconnect events
  if (_connectionAttempted && !_connectInProgress) {
    _client.loop();
  }

  // Handle manual reconnection with exponential backoff
  // Only if not manually disconnected and not already connecting
  if (!_connected && _connectionAttempted && !_manuallyDisconnected &&
      _autoReconnect && _serverHost.length() > 0 && !_connectInProgress) {
    unsigned long now = millis();

    // Calculate backoff interval: base * 2^failures, capped at 60 seconds
    unsigned long backoff =
        _reconnectInterval * (1 << min(_consecutiveFailures, (unsigned int)3));
    if (backoff > 60000)
      backoff = 60000;

    if (now - _lastReconnectAttempt > backoff) {
      _lastReconnectAttempt = now;
      _connectInProgress = true;
      _consecutiveFailures++;

      Serial.print("Auto-reconnect: Attempting in ");
      Serial.print(backoff / 1000);
      Serial.print("s (attempt #");
      Serial.print(_consecutiveFailures);
      Serial.println(")");

      // Retry the connection
      bool isSocketIO = (_serverPath.indexOf("/socket.io") >= 0);
      if (isSocketIO) {
        String socketIOPath = _serverPath + "?EIO=4&transport=websocket";
        _client.begin(_serverHost.c_str(), _serverPort, socketIOPath.c_str());
      } else {
        _client.begin(_serverHost.c_str(), _serverPort, _serverPath.c_str());
      }

      // Reset connect in progress after a short delay
      // This prevents the tight loop while allowing the library to process
      _connectInProgress = false;
    }
  }
}

const char *WebSocketClient::getStatus() {
  if (_connected) {
    return "Connected";
  } else if (_manuallyDisconnected) {
    return "Disconnected"; // Don't show "Reconnecting..." if manually
                           // disconnected
  } else if (_connectionAttempted && _autoReconnect &&
             _serverHost.length() > 0) {
    return "Reconnecting...";
  } else {
    return "Not connected";
  }
}

const char *WebSocketClient::getServerUrl() { return _fullUrl.c_str(); }

// Static callback function
void WebSocketClient::webSocketEvent(WStype_t type, uint8_t *payload,
                                     size_t length) {
  if (_instance) {
    _instance->handleWebSocketEvent(type, payload, length);
  }
}

void WebSocketClient::handleWebSocketEvent(WStype_t type, uint8_t *payload,
                                           size_t length) {
  switch (type) {
  case WStype_DISCONNECTED:
    // Rate limit disconnect logging to prevent flood
    {
      static unsigned long lastDisconnectLog = 0;
      unsigned long now = millis();
      if (now - lastDisconnectLog > 10000) { // Log at most every 10 seconds
        lastDisconnectLog = now;
        Serial.println("WebSocket: Connection failed/disconnected");
      }
    }
    _connected = false;
    _connectInProgress = false; // Allow new connection attempts

    // If this was a manual disconnect, ensure we stay disconnected
    if (_manuallyDisconnected) {
      _connectionAttempted = false; // Prevent any reconnection attempts
      _consecutiveFailures = 0;     // Reset failure counter
    }
    break;

  case WStype_CONNECTED:
    Serial.print("WebSocket: Connected to: ");
    Serial.println((char *)payload);
    _connected = true;
    _connectInProgress = false;
    _consecutiveFailures = 0; // Reset failure counter on successful connection

    // Connection is managed by the library
    Serial.println("WebSocket: Ready to receive timer events");
    break;

  case WStype_TEXT: {
    // Force connected state if we receive data (in case CONNECTED event was
    // missed)
    if (!_connected) {
      _connected = true;
      _consecutiveFailures = 0;

      // Also define start time for reconnect backoff just in case
      _lastReconnectAttempt = millis();

      Serial.println("WebSocket: Connected (inferred from data)");
    }

    String data = String((char *)payload);
    DEBUG_PRINT("Message received: ");
    DEBUG_PRINTLN(data);

    // Handle Socket.IO protocol messages
    if (data.startsWith("0")) {
      DEBUG_PRINTLN("Socket.IO: Connection request");
      // Send connection response
      _client.sendTXT("40");
      return;
    } else if (data.startsWith("40")) {
      DEBUG_PRINTLN("Socket.IO: Connected successfully");
      return;
    } else if (data.startsWith("2")) {
      DEBUG_PRINTLN("Socket.IO: Ping - sending pong");
      _client.sendTXT("3");
      return;
    } else if (data.startsWith("42")) {
      DEBUG_PRINTLN("Socket.IO: Event message");
      // Extract JSON from Socket.IO event format: 42["event_name", data]
      int bracketPos = data.indexOf('[');
      if (bracketPos > 0) {
        data = data.substring(bracketPos);
      }
    } else if (!data.startsWith("{") && !data.startsWith("[")) {
      DEBUG_PRINT("Socket.IO: Unknown packet type - ");
      DEBUG_PRINTLN(data.substring(0, 2));
      return;
    }

    // Parse JSON message
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);

    if (error) {
      DEBUG_PRINT("JSON parse error: ");
      DEBUG_PRINTLN(error.c_str());
      DEBUG_PRINT("Raw data: ");
      DEBUG_PRINTLN(data);
      return;
    }

    // Check if this is a timer_update event
    // Socket.IO format can vary, handle both direct and wrapped formats
    if (doc.is<JsonArray>()) {
      JsonArray arr = doc.as<JsonArray>();
      if (arr.size() >= 2 && arr[0] == "timer_update" &&
          arr[1].is<JsonObject>()) {
        DEBUG_PRINTLN("Processing timer_update event");
        JsonObject obj = arr[1];
        handleTimerUpdate(obj);
      }
    } else if (doc["timer_update"].is<JsonObject>()) {
      JsonObject obj = doc["timer_update"];
      handleTimerUpdate(obj);
    } else if (doc["action"].is<const char *>()) {
      JsonObject obj = doc.as<JsonObject>();
      handleTimerUpdate(obj);
    }
  } break;

  case WStype_BIN:
    DEBUG_PRINTLN("WebSocket binary message received (ignored)");
    break;

  case WStype_PING:
    DEBUG_PRINTLN("WebSocket ping received");
    break;

  case WStype_PONG:
    DEBUG_PRINTLN("WebSocket pong received");
    break;

  case WStype_ERROR:
    DEBUG_PRINTLN("WebSocket error occurred");
    _connected = false;
    break;

  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    DEBUG_PRINTLN("WebSocket fragment received");
    break;
  }
}

void WebSocketClient::handleTimerUpdate(JsonObject &obj) {
  const char *action = obj["action"];

  if (action == nullptr) {
    DEBUG_PRINTLN("No action field in timer_update");
    return;
  }

  DEBUG_PRINT("Timer action: ");
  DEBUG_PRINTLN(action);

  if (strcmp(action, "start") == 0) {
    // Just start the timer - duration setting and reset are handled by reset
    // events
    DEBUG_PRINTLN("Starting timer (resume if paused, or start if reset)");
    _timer->start();

  } else if (strcmp(action, "stop") == 0) {
    DEBUG_PRINTLN("Stopping timer");
    _timer->stop();

  } else if (strcmp(action, "reset") == 0) {
    int minutes = obj["minutes"] | 3;
    int seconds = obj["seconds"] | 0;

    DEBUG_PRINT("Resetting timer: ");
    DEBUG_PRINT(minutes);
    DEBUG_PRINT(":");
    if (seconds < 10)
      DEBUG_PRINT("0");
    DEBUG_PRINTLN(seconds);

    // Set duration and reset - timer will stop and not auto-restart
    _timer->setDuration({(unsigned int)minutes, (unsigned int)seconds, 0});
    _timer->reset();

  } else if (strcmp(action, "settings") == 0) {
    // Handle settings update
    JsonObject settings = obj["settings"];

    if (!settings.isNull()) {
      // Could update display settings here if needed
      // For now, we'll just log it
      DEBUG_PRINTLN("Settings update received (not applied to physical timer)");

      // Optionally extract endMessage or other relevant settings
      if (settings["endMessage"].is<const char *>()) {
        const char *endMsg = settings["endMessage"];
        DEBUG_PRINT("End message: ");
        DEBUG_PRINTLN(endMsg);
        // Could call _timer->setEndMessage(endMsg) if that method exists
      }
    }
  }
}
