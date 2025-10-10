#include "WebSocketClient.h"

// Debug flag - set to false to disable debug messages for better timing
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
WebSocketClient* WebSocketClient::_instance = nullptr;

WebSocketClient::WebSocketClient(Timer* timer)
    : _timer(timer), _connected(false), _connectionAttempted(false),
      _lastReconnectAttempt(0), _reconnectInterval(5000), 
      _autoReconnect(true), _serverPort(8765) {
    
    // Set instance for static callback
    _instance = this;
    
    // Set up event handler
    _client.onEvent(webSocketEvent);
    
    // Disable SSL verification (not needed for local connections)
    _client.setReconnectInterval(_reconnectInterval);
}

bool WebSocketClient::connect(const char* host, uint16_t port, const char* path) {
    if (_connected) {
        disconnect();
    }
    
    _connectionAttempted = true;  // Mark that user has attempted connection
    _serverHost = String(host);
    _serverPort = port;
    _serverPath = String(path);
    
    // Build URL for display
    _fullUrl = "ws://" + _serverHost + ":" + String(_serverPort) + _serverPath;
    
    DEBUG_PRINT("Connecting to WebSocket server: ");
    DEBUG_PRINTLN(_fullUrl);
    DEBUG_PRINTLN("NOTE: Socket.IO v3+ is not fully supported by this library.");
    DEBUG_PRINTLN("For best results, use a plain WebSocket server or Socket.IO v2.");
    
    // Connect using plain WebSocket (Socket.IO v3+ not supported by beginSocketIO)
    _client.begin(_serverHost.c_str(), _serverPort, _serverPath.c_str());
    
    // Connection result will come via callback
    DEBUG_PRINTLN("WebSocket connection initiated...");
    
    return true;  // Actual connection status will be updated via callback
}

void WebSocketClient::disconnect() {
    if (_connected) {
        _client.disconnect();
        _connected = false;
        DEBUG_PRINTLN("WebSocket disconnected");
    }
    _connectionAttempted = false;  // Clear connection attempt flag
}

bool WebSocketClient::isConnected() {
    return _connected;
}

void WebSocketClient::poll() {
    _client.loop();
}

const char* WebSocketClient::getStatus() {
    if (_connected) {
        return "Connected";
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
            DEBUG_PRINTLN("WebSocket disconnected");
            _connected = false;
            break;
            
        case WStype_CONNECTED:
            DEBUG_PRINT("Socket.IO connected to: ");
            DEBUG_PRINTLN((char*)payload);
            _connected = true;
            
            // Socket.IO client - connection is managed by the library
            // No need to send manual connection message
            break;
            
        case WStype_TEXT:
            {
                String data = String((char*)payload);
                DEBUG_PRINT("WebSocket message received: ");
                DEBUG_PRINTLN(data);
                
                // Parse JSON message
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, data);
                
                if (error) {
                    DEBUG_PRINT("JSON parse error: ");
                    DEBUG_PRINTLN(error.c_str());
                    return;
                }
                
                // Check if this is a timer_update event
                // Socket.IO format can vary, handle both direct and wrapped formats
                if (doc["timer_update"].is<JsonObject>()) {
                    JsonObject obj = doc["timer_update"];
                    handleTimerUpdate(obj);
                } else if (doc["action"].is<const char*>()) {
                    JsonObject obj = doc.as<JsonObject>();
                    handleTimerUpdate(obj);
                }
                // Socket.IO may wrap in array format: ["timer_update", {...}]
                else if (doc.is<JsonArray>()) {
                    JsonArray arr = doc.as<JsonArray>();
                    if (arr.size() >= 2 && arr[0] == "timer_update" && arr[1].is<JsonObject>()) {
                        JsonObject obj = arr[1];
                        handleTimerUpdate(obj);
                    }
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
        Serial.println("Stopping timer");
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
            Serial.println("Timer was actively running, restarting after reset");
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
