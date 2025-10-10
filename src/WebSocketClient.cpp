#include "WebSocketClient.h"

// Static instance pointer for callback
WebSocketClient* WebSocketClient::_instance = nullptr;

WebSocketClient::WebSocketClient(Timer* timer)
    : _timer(timer), _connected(false), _lastReconnectAttempt(0), 
      _reconnectInterval(5000), _autoReconnect(true), _serverPort(8765) {
    
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
    
    _serverHost = String(host);
    _serverPort = port;
    _serverPath = String(path);
    
    // Build URL for display
    _fullUrl = "ws://" + _serverHost + ":" + String(_serverPort) + _serverPath;
    
    Serial.print("Connecting to WebSocket: ");
    Serial.println(_fullUrl);
    
    // Connect to WebSocket server
    _client.begin(_serverHost.c_str(), _serverPort, _serverPath.c_str());
    
    // Connection result will come via callback
    Serial.println("WebSocket connection initiated...");
    
    return true;  // Actual connection status will be updated via callback
}

void WebSocketClient::disconnect() {
    if (_connected) {
        _client.disconnect();
        _connected = false;
        Serial.println("WebSocket disconnected");
    }
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
    } else if (_autoReconnect && _serverHost.length() > 0) {
        return "Reconnecting...";
    } else {
        return "Disconnected";
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
            Serial.println("WebSocket disconnected");
            _connected = false;
            break;
            
        case WStype_CONNECTED:
            Serial.print("WebSocket connected to: ");
            Serial.println((char*)payload);
            _connected = true;
            
            // Send initial connection message for Socket.IO compatibility
            _client.sendTXT("{\"type\":\"connect\"}");
            break;
            
        case WStype_TEXT:
            {
                String data = String((char*)payload);
                Serial.print("WebSocket message received: ");
                Serial.println(data);
                
                // Parse JSON message
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, data);
                
                if (error) {
                    Serial.print("JSON parse error: ");
                    Serial.println(error.c_str());
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
            Serial.println("WebSocket binary message received (ignored)");
            break;
            
        case WStype_PING:
            Serial.println("WebSocket ping received");
            break;
            
        case WStype_PONG:
            Serial.println("WebSocket pong received");
            break;
            
        case WStype_ERROR:
            Serial.println("WebSocket error occurred");
            _connected = false;
            break;
            
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            Serial.println("WebSocket fragment received");
            break;
    }
}

void WebSocketClient::handleTimerUpdate(JsonObject& obj) {
    const char* action = obj["action"];
    
    if (action == nullptr) {
        Serial.println("No action field in timer_update");
        return;
    }
    
    Serial.print("Timer action: ");
    Serial.println(action);
    
    if (strcmp(action, "start") == 0) {
        // Start timer with specified time
        int minutes = obj["minutes"] | 3;  // Default 3 minutes
        int seconds = obj["seconds"] | 0;  // Default 0 seconds
        
        Serial.printf("Starting timer: %d:%02d\n", minutes, seconds);
        
        _timer->setDuration({(unsigned int)minutes, (unsigned int)seconds, 0});
        _timer->reset();
        _timer->start();
        
    } else if (strcmp(action, "stop") == 0) {
        Serial.println("Stopping timer");
        _timer->stop();
        
    } else if (strcmp(action, "reset") == 0) {
        int minutes = obj["minutes"] | 3;
        int seconds = obj["seconds"] | 0;
        
        Serial.printf("Resetting timer: %d:%02d\n", minutes, seconds);
        
        _timer->setDuration({(unsigned int)minutes, (unsigned int)seconds, 0});
        _timer->reset();
        
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
