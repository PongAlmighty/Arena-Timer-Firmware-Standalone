/**
 * SimpleWebSocketClient.h - Minimal WebSocket client for Ethernet
 *
 * A lightweight WebSocket client implementation that works with any
 * Arduino-compatible Client. Uses dependency injection - you pass in
 * the Client object from your main code.
 *
 * Supports:
 * - WebSocket connection handshake (RFC 6455)
 * - Socket.IO protocol (for FightTimer compatibility)
 * - Text message sending/receiving
 * - Ping/Pong for keepalive
 * - Automatic reconnection
 *
 * For use with Arena Timer to receive timer updates from FightTimer server.
 */

#ifndef SIMPLE_WEBSOCKET_CLIENT_H
#define SIMPLE_WEBSOCKET_CLIENT_H

#include <Arduino.h>
#include <Client.h>

// WebSocket opcodes (RFC 6455)
#define WS_OPCODE_CONTINUATION 0x00
#define WS_OPCODE_TEXT 0x01
#define WS_OPCODE_BINARY 0x02
#define WS_OPCODE_CLOSE 0x08
#define WS_OPCODE_PING 0x09
#define WS_OPCODE_PONG 0x0A

// Connection states
enum WSState { WS_DISCONNECTED, WS_CONNECTING, WS_CONNECTED, WS_CLOSING };

// Callback type for received messages
typedef void (*WSMessageCallback)(const char *message, size_t length);

class SimpleWebSocketClient {
public:
  /**
   * Constructor - takes a reference to a Client object
   * The Client must remain valid for the lifetime of this object
   */
  SimpleWebSocketClient(Client &client);

  /**
   * Connect to a WebSocket server
   * @param host Server hostname or IP
   * @param port Server port (default 80)
   * @param path WebSocket path (default "/")
   * @return true if connection initiated
   */
  bool connect(const char *host, uint16_t port = 80, const char *path = "/");

  /**
   * Disconnect from server
   */
  void disconnect();

  /**
   * Check if connected
   */
  bool isConnected();

  /**
   * Get current state
   */
  WSState getState() { return _state; }

  /**
   * Poll for incoming messages (call in loop)
   * Returns true if a message was received
   */
  bool poll();

  /**
   * Send a text message
   * @param message Text to send
   * @return true if sent successfully
   */
  bool sendText(const char *message);

  /**
   * Set callback for received messages
   */
  void onMessage(WSMessageCallback callback) { _messageCallback = callback; }

  /**
   * Enable/disable auto-reconnect
   */
  void setAutoReconnect(bool enabled, unsigned long intervalMs = 5000);

  /**
   * Enable Socket.IO mode (for FightTimer server compatibility)
   * When enabled, handles Socket.IO protocol on top of WebSocket
   */
  void setSocketIOMode(bool enabled) { _socketIOMode = enabled; }

  /**
   * Get server URL for status display
   */
  String getServerUrl();

private:
  Client &_client; // Reference to external Client
  WSState _state;

  // Connection info
  String _host;
  uint16_t _port;
  String _path;

  // Callbacks
  WSMessageCallback _messageCallback;

  // Auto-reconnect
  bool _autoReconnect;
  unsigned long _reconnectInterval;
  unsigned long _lastReconnectAttempt;

  // Ping/Pong keepalive
  unsigned long _lastPingTime;
  unsigned long _pingInterval;
  bool _awaitingPong;

  // Socket.IO mode
  bool _socketIOMode;
  bool _socketIOConnected;

  // Message buffer
  static const size_t BUFFER_SIZE = 512;
  char _rxBuffer[BUFFER_SIZE];
  size_t _rxBufferPos;

  // Internal methods
  bool performHandshake();
  bool readFrame(uint8_t &opcode, char *payload, size_t &length);
  bool sendFrame(uint8_t opcode, const char *payload, size_t length);
  void handlePing(const char *payload, size_t length);
  void handleClose();
  String generateWebSocketKey();
  String base64Encode(const uint8_t *data, size_t length);

  // Socket.IO helpers
  bool handleSocketIOMessage(const char *message, size_t length);
  void sendSocketIOPacket(const char *packet);
};

#endif // SIMPLE_WEBSOCKET_CLIENT_H
