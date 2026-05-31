#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include "Timer.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <WebSocketsClient.h>

class WebSocketClient {
public:
  WebSocketClient(Timer *timer);

  // Connection management
  bool connect(const char *host, uint16_t port,
               const char *path = "/socket.io/", const char *ns = "/");
  void disconnect();
  bool isConnected();

  // Must be called in loop()
  void poll();

  // Status
  const char *getStatus();
  const char *getServerUrl();

  // Timer control (Phase 2: local stub; Phase 3: emits Socket.IO then calls local)
  void emitTimerControl(const char* action);

private:
  Timer *_timer;
  WebSocketsClient _client;

  String _serverHost;
  uint16_t _serverPort;
  String _serverPath;
  String _namespace;
  String _fullUrl;

  bool _connected;
  bool _connectionAttempted;  // Track if user has tried to connect
  bool _manuallyDisconnected; // Track if user manually disconnected
  bool _connectInProgress;    // Prevent overlapping connection attempts
  unsigned long _lastReconnectAttempt;
  unsigned long _reconnectInterval;
  unsigned int _consecutiveFailures; // For exponential backoff
  bool _autoReconnect;

  // Phase 3: Socket.IO bidirectional sync state
  String _suppressAction;          // action string being echo-suppressed ("" = not suppressing)
  unsigned long _suppressUntilMs;  // millis() expiry of suppression window (SYNC-02)
  bool _initialSyncPending;        // true = next start event skips request_timer_status round-trip (SYNC-03)

  // Socket.IO support
  bool _isSocketIO;
  bool _socketIOFallback; // Try WebSocket if Socket.IO fails
  String _socketIOSessionId;

  // Event handler
  static void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
  static WebSocketClient *_instance; // For static callback

  void handleWebSocketEvent(WStype_t type, uint8_t *payload, size_t length);

  // Message parsing
  void handleTimerUpdate(JsonObject &obj);

  // Persistence
  void loadSettings();
  void saveSettings();

public:
  // Getters for UI persistence
  String getHost() { return _serverHost; }
  uint16_t getPort() { return _serverPort; }
  String getPath() { return _serverPath; }
  String getNamespace() { return _namespace; }

private:
#ifdef ARDUINO_ARCH_RP2040
  // Use Preferences if available (Standard on arduino-pico)
  // Determine if we need to include specific header or if it's available
#endif
};

#endif // WEBSOCKET_CLIENT_H
