#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "Timer.h"

class WebSocketClient {
public:
    WebSocketClient(Timer* timer);
    
    // Connection management
    bool connect(const char* host, uint16_t port, const char* path = "/socket.io/");
    void disconnect();
    bool isConnected();
    
    // Must be called in loop()
    void poll();
    
    // Status
    const char* getStatus();
    const char* getServerUrl();
    
private:
    Timer* _timer;
    WebSocketsClient _client;
    
    String _serverHost;
    uint16_t _serverPort;
    String _serverPath;
    String _fullUrl;
    
    bool _connected;
    bool _connectionAttempted;  // Track if user has tried to connect
    unsigned long _lastReconnectAttempt;
    unsigned long _reconnectInterval;
    bool _autoReconnect;
    
    // Event handler
    static void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    static WebSocketClient* _instance;  // For static callback
    
    void handleWebSocketEvent(WStype_t type, uint8_t * payload, size_t length);
    
    // Message parsing
    void handleTimerUpdate(JsonObject& obj);
};

#endif // WEBSOCKET_CLIENT_H
