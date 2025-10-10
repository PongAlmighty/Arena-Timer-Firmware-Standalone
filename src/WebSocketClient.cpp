#include "WebSocketClient.h"

// Debug flag - set to false to disable debug messages for better timing
#define DEBUG_WEBSOCKET false

// Debug printing macros
#if DEBUG_WEBSOCKET
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif

// Static instance pointer for callback
WebSocketClient* WebSocketClient::_instance = nullptr;

WebSocketClient::WebSocketClient(Timer* timer)
    : _timer(timer), _connected(false), _connectionAttempted(false),
      _manuallyDisconnected(false), _lastReconnectAttempt(0), _reconnectInterval(5000), 
      _autoReconnect(true), _serverPort(8765) {
    
    // Set instance for static callback
    _instance = this;
    
    // Set up event handler
    _client.onEvent(webSocketEvent);
    
    // Disable SSL verification (not needed for local connections)
    // Disable library auto-reconnect - we'll handle it manually
    _client.setReconnectInterval(0);
}

bool WebSocketClient::connect(const char* host, uint16_t port, const char* path) {
    if (_connected) {
        disconnect();
    }
    
    _connectionAttempted = true;  // Mark that user has attempted connection
    _manuallyDisconnected = false;  // Clear manual disconnect flag when reconnecting
    _serverHost = String(host);
    _serverPort = port;
    _serverPath = String(path);
    
    // Build URL for display
    _fullUrl = "ws://" + _serverHost + ":" + String(_serverPort) + _serverPath;
    
    DEBUG_PRINT("Connecting to server: ");
    DEBUG_PRINTLN(_fullUrl);
    
    // Check if this looks like a Socket.IO connection
    bool isSocketIO = (_serverPath.indexOf("/socket.io") >= 0);
    
    if (isSocketIO) {
        DEBUG_PRINTLN("Detected Socket.IO path - trying direct WebSocket connection");
        DEBUG_PRINTLN("Socket.IO will be handled at message level");
        
        // Try connecting directly to the Socket.IO WebSocket transport
        // FightTimer might be using Socket.IO v4+ which needs EIO=4 parameter
        String socketIOPath = _serverPath + "?EIO=4&transport=websocket";
        DEBUG_PRINT("Using path: ");
        DEBUG_PRINTLN(socketIOPath);
        
        _client.begin(_serverHost.c_str(), _serverPort, socketIOPath.c_str());
    } else {
        DEBUG_PRINTLN("Using standard WebSocket connection");
        // For regular WebSocket, use standard begin
        _client.begin(_serverHost.c_str(), _serverPort, _serverPath.c_str());
    }
    
    // Connection result will come via callback
    DEBUG_PRINTLN("Connection initiated...");
    
    return true;  // Actual connection status will be updated via callback
}

void WebSocketClient::disconnect() {
    DEBUG_PRINTLN("Disconnect requested...");
    
    // Set flags first to prevent any race conditions
    _manuallyDisconnected = true;  // Mark as manually disconnected
    _connectionAttempted = false;  // Clear connection attempt flag
    _connected = false;  // Set disconnected state
    
    // Now disconnect from the WebSocket
    _client.disconnect();
    
    // Force stop any internal reconnection by reinitializing the client
    _client = WebSocketsClient();  // Reset the client
    _client.onEvent(webSocketEvent);  // Reattach event handler
    _client.setReconnectInterval(0);  // Disable auto-reconnect
    
    DEBUG_PRINTLN("WebSocket forcibly disconnected and reset");
    DEBUG_PRINT("Manual disconnect flag set: ");
    DEBUG_PRINTLN(_manuallyDisconnected ? "true" : "false");
}

bool WebSocketClient::isConnected() {
    return _connected;
}

void WebSocketClient::poll() {
    _client.loop();
    
    // Handle manual reconnection (only if not manually disconnected)
    if (!_connected && _connectionAttempted && !_manuallyDisconnected && _autoReconnect && _serverHost.length() > 0) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > _reconnectInterval) {
            _lastReconnectAttempt = now;
            DEBUG_PRINTLN("Auto-reconnect: Attempting to reconnect...");
            DEBUG_PRINT("Manual disconnect flag: ");
            DEBUG_PRINTLN(_manuallyDisconnected ? "true" : "false");
            
            // Retry the connection
            bool isSocketIO = (_serverPath.indexOf("/socket.io") >= 0);
            if (isSocketIO) {
                String socketIOPath = _serverPath + "?EIO=4&transport=websocket";
                _client.begin(_serverHost.c_str(), _serverPort, socketIOPath.c_str());
            } else {
                _client.begin(_serverHost.c_str(), _serverPort, _serverPath.c_str());
            }
        }
    }
}

const char* WebSocketClient::getStatus() {
    if (_connected) {
        return "Connected";
    } else if (_manuallyDisconnected) {
        return "Disconnected";  // Don't show "Reconnecting..." if manually disconnected
    } else if (_connectionAttempted && _autoReconnect && _serverHost.length() > 0) {
        return "Reconnecting...";
    } else {
        return "Not connected";
    }
}

const char* WebSocketClient::getServerUrl() {
    return _fullUrl.c_str();
}

// Static callback function
void WebSocketClient::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    if (_instance) {
        _instance->handleWebSocketEvent(type, payload, length);
    }
}

void WebSocketClient::handleWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            DEBUG_PRINTLN("WebSocket disconnected event received");
            DEBUG_PRINT("Was manually disconnected: ");
            DEBUG_PRINTLN(_manuallyDisconnected ? "true" : "false");
            _connected = false;
            
            // If this was a manual disconnect, ensure we stay disconnected
            if (_manuallyDisconnected) {
                DEBUG_PRINTLN("Manual disconnect - ensuring no reconnection");
                _connectionAttempted = false;  // Prevent any reconnection attempts
            }
            break;
            
        case WStype_CONNECTED:
            DEBUG_PRINT("Connected to: ");
            DEBUG_PRINTLN((char*)payload);
            _connected = true;
            
            // Connection is managed by the library
            DEBUG_PRINTLN("Ready to receive timer events");
            break;
            
        case WStype_TEXT:
            {
                String data = String((char*)payload);
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
                    if (arr.size() >= 2 && arr[0] == "timer_update" && arr[1].is<JsonObject>()) {
                        DEBUG_PRINTLN("Processing timer_update event");
                        JsonObject obj = arr[1];
                        handleTimerUpdate(obj);
                    }
                } else if (doc["timer_update"].is<JsonObject>()) {
                    JsonObject obj = doc["timer_update"];
                    handleTimerUpdate(obj);
                } else if (doc["action"].is<const char*>()) {
                    JsonObject obj = doc.as<JsonObject>();
                    handleTimerUpdate(obj);
                }
            }
            break;
            
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

void WebSocketClient::handleTimerUpdate(JsonObject& obj) {
    const char* action = obj["action"];
    
    if (action == nullptr) {
        DEBUG_PRINTLN("No action field in timer_update");
        return;
    }
    
    DEBUG_PRINT("Timer action: ");
    DEBUG_PRINTLN(action);
    
    if (strcmp(action, "start") == 0) {
        // Just start the timer - duration setting and reset are handled by reset events
        DEBUG_PRINTLN("Starting timer (resume if paused, or start if reset)");
        _timer->start();
        
    } else if (strcmp(action, "stop") == 0) {
        DEBUG_PRINTLN("Stopping timer");
        _timer->stop();
        
    } else if (strcmp(action, "reset") == 0) {
        int minutes = obj["minutes"] | 3;
        int seconds = obj["seconds"] | 0;
        
        Serial.printf("Resetting timer: %d:%02d\n", minutes, seconds);
        
        // Check if timer was actively running before reset (not expired)
        // Only restart if it was running and not expired (FightTimer behavior)
        bool wasRunning = _timer->isRunning() && !_timer->isPaused() && !_timer->isExpired();
        
        _timer->setDuration({(unsigned int)minutes, (unsigned int)seconds, 0});
        _timer->reset();
        
        // If timer was actively running (not expired), restart it (mimic FightTimer behavior)
        if (wasRunning) {
            DEBUG_PRINTLN("Timer was actively running, restarting after reset");
            _timer->start();
        }
        
    } else if (strcmp(action, "settings") == 0) {
        // Handle settings update
        JsonObject settings = obj["settings"];
        
        if (!settings.isNull()) {
            // Could update display settings here if needed
            // For now, we'll just log it
            Serial.println("Settings update received (not applied to physical timer)");
            
            // Optionally extract endMessage or other relevant settings
            if (settings["endMessage"].is<const char*>()) {
                const char* endMsg = settings["endMessage"];
                Serial.print("End message: ");
                Serial.println(endMsg);
                // Could call _timer->setEndMessage(endMsg) if that method exists
            }
        }
    }
}
