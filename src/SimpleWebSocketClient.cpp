/**
 * SimpleWebSocketClient.cpp - Minimal WebSocket client implementation
 *
 * Implements RFC 6455 WebSocket protocol for Arduino.
 * Self-contained, no external dependencies beyond Arduino core.
 */

#include "SimpleWebSocketClient.h"

SimpleWebSocketClient::SimpleWebSocketClient(Client &client)
    : _client(client), _state(WS_DISCONNECTED), _port(80),
      _messageCallback(nullptr), _autoReconnect(false),
      _reconnectInterval(5000), _lastReconnectAttempt(0), _lastPingTime(0),
      _pingInterval(30000), // Send ping every 30 seconds
      _awaitingPong(false), _socketIOMode(false), _socketIOConnected(false),
      _rxBufferPos(0) {}

bool SimpleWebSocketClient::connect(const char *host, uint16_t port,
                                    const char *path) {
  _host = host;
  _port = port;
  _path = path;

  Serial.print("[WS] Connecting to ");
  Serial.print(host);
  Serial.print(":");
  Serial.println(port);

  _state = WS_CONNECTING;

  // TCP connect
  if (!_client.connect(host, port)) {
    Serial.println("[WS] TCP connection failed");
    _state = WS_DISCONNECTED;
    return false;
  }

  // Perform WebSocket handshake
  if (!performHandshake()) {
    Serial.println("[WS] Handshake failed");
    _client.stop();
    _state = WS_DISCONNECTED;
    return false;
  }

  Serial.println("[WS] Connected!");
  _state = WS_CONNECTED;
  _lastPingTime = millis();
  _awaitingPong = false;

  return true;
}

void SimpleWebSocketClient::disconnect() {
  if (_state == WS_CONNECTED) {
    // Send close frame
    sendFrame(WS_OPCODE_CLOSE, nullptr, 0);
  }
  _client.stop();
  _state = WS_DISCONNECTED;
  Serial.println("[WS] Disconnected");
}

bool SimpleWebSocketClient::isConnected() {
  return _state == WS_CONNECTED && _client.connected();
}

bool SimpleWebSocketClient::poll() {
  // Handle auto-reconnect
  if (_autoReconnect && _state == WS_DISCONNECTED) {
    if (millis() - _lastReconnectAttempt >= _reconnectInterval) {
      _lastReconnectAttempt = millis();
      Serial.println("[WS] Attempting reconnect...");
      connect(_host.c_str(), _port, _path.c_str());
    }
    return false;
  }

  // Check connection
  if (!_client.connected()) {
    if (_state == WS_CONNECTED) {
      Serial.println("[WS] Connection lost");
      _state = WS_DISCONNECTED;
      _socketIOConnected = false;
    }
    return false;
  }

  // Send periodic ping (only for non-Socket.IO mode, Socket.IO has its own)
  if (!_socketIOMode && millis() - _lastPingTime >= _pingInterval) {
    if (_awaitingPong) {
      Serial.println("[WS] Pong timeout, disconnecting");
      disconnect();
      return false;
    }
    sendFrame(WS_OPCODE_PING, nullptr, 0);
    _lastPingTime = millis();
    _awaitingPong = true;
  }

  // Read incoming frames
  if (_client.available()) {
    uint8_t opcode;
    size_t length = BUFFER_SIZE - 1;

    if (readFrame(opcode, _rxBuffer, length)) {
      _rxBuffer[length] = '\0'; // Null-terminate

      switch (opcode) {
      case WS_OPCODE_TEXT:
        if (_socketIOMode) {
          // Handle Socket.IO protocol
          return handleSocketIOMessage(_rxBuffer, length);
        } else {
          // Raw WebSocket mode
          if (_messageCallback) {
            _messageCallback(_rxBuffer, length);
          }
          return true;
        }

      case WS_OPCODE_PING:
        handlePing(_rxBuffer, length);
        break;

      case WS_OPCODE_PONG:
        _awaitingPong = false;
        break;

      case WS_OPCODE_CLOSE:
        handleClose();
        break;
      }
    }
  }

  return false;
}

bool SimpleWebSocketClient::sendText(const char *message) {
  if (_state != WS_CONNECTED) {
    return false;
  }
  return sendFrame(WS_OPCODE_TEXT, message, strlen(message));
}

void SimpleWebSocketClient::setAutoReconnect(bool enabled,
                                             unsigned long intervalMs) {
  _autoReconnect = enabled;
  _reconnectInterval = intervalMs;
}

String SimpleWebSocketClient::getServerUrl() {
  return "ws://" + _host + ":" + String(_port) + _path;
}

// ============ Private Methods ============

bool SimpleWebSocketClient::performHandshake() {
  // Generate random key
  String key = generateWebSocketKey();

  // Send HTTP upgrade request
  _client.print("GET ");
  _client.print(_path);
  _client.println(" HTTP/1.1");
  _client.print("Host: ");
  _client.println(_host);
  _client.println("Upgrade: websocket");
  _client.println("Connection: Upgrade");
  _client.print("Sec-WebSocket-Key: ");
  _client.println(key);
  _client.println("Sec-WebSocket-Version: 13");
  _client.println();

  // Wait for response (with timeout)
  unsigned long start = millis();
  while (!_client.available()) {
    if (millis() - start > 5000) {
      Serial.println("[WS] Handshake timeout");
      return false;
    }
    delay(10);
  }

  // Read response - need to buffer it since Client doesn't have readStringUntil
  char response[256];
  int idx = 0;
  start = millis();
  while (millis() - start < 2000 && idx < 255) {
    if (_client.available()) {
      char c = _client.read();
      response[idx++] = c;
      if (c == '\n' && idx >= 2 && response[idx - 2] == '\r') {
        break;
      }
    }
  }
  response[idx] = '\0';

  // Check for "101 Switching Protocols"
  if (strstr(response, "101") == nullptr) {
    Serial.print("[WS] Bad response: ");
    Serial.println(response);
    return false;
  }

  // Skip remaining headers (read until blank line)
  bool foundEnd = false;
  int consecutiveNewlines = 0;
  start = millis();
  while (!foundEnd && millis() - start < 2000) {
    if (_client.available()) {
      char c = _client.read();
      if (c == '\n') {
        consecutiveNewlines++;
        if (consecutiveNewlines >= 2) {
          foundEnd = true;
        }
      } else if (c != '\r') {
        consecutiveNewlines = 0;
      }
    }
  }

  return true;
}

bool SimpleWebSocketClient::readFrame(uint8_t &opcode, char *payload,
                                      size_t &maxLength) {
  if (_client.available() < 2) {
    return false;
  }

  // Read first two bytes
  uint8_t byte1 = _client.read();
  uint8_t byte2 = _client.read();

  bool fin = (byte1 & 0x80) != 0;
  opcode = byte1 & 0x0F;
  bool masked = (byte2 & 0x80) != 0;
  size_t length = byte2 & 0x7F;

  // Extended length
  if (length == 126) {
    if (_client.available() < 2)
      return false;
    length = (_client.read() << 8) | _client.read();
  } else if (length == 127) {
    // 64-bit length - skip high bytes, read low 4
    if (_client.available() < 8)
      return false;
    for (int i = 0; i < 4; i++)
      _client.read(); // Skip high bytes
    length = 0;
    for (int i = 0; i < 4; i++) {
      length = (length << 8) | _client.read();
    }
  }

  // Read mask if present
  uint8_t mask[4] = {0, 0, 0, 0};
  if (masked) {
    if (_client.available() < 4)
      return false;
    for (int i = 0; i < 4; i++) {
      mask[i] = _client.read();
    }
  }

  // Limit to buffer size
  size_t toRead = min(length, maxLength);

  // Read payload
  size_t bytesRead = 0;
  unsigned long start = millis();
  while (bytesRead < toRead && (millis() - start) < 1000) {
    if (_client.available()) {
      uint8_t b = _client.read();
      if (masked) {
        b ^= mask[bytesRead % 4];
      }
      payload[bytesRead++] = b;
    }
  }

  // Discard remaining if truncated
  while (bytesRead < length && _client.available()) {
    _client.read();
    bytesRead++;
  }

  maxLength = min(toRead, bytesRead);
  return true;
}

bool SimpleWebSocketClient::sendFrame(uint8_t opcode, const char *payload,
                                      size_t length) {
  if (!_client.connected()) {
    return false;
  }

  // Client frames must be masked (RFC 6455)
  uint8_t mask[4];
  for (int i = 0; i < 4; i++) {
    mask[i] = random(256);
  }

  // First byte: FIN + opcode
  _client.write(0x80 | opcode);

  // Second byte: MASK bit + length
  if (length < 126) {
    _client.write(0x80 | (uint8_t)length);
  } else if (length < 65536) {
    _client.write(0x80 | 126);
    _client.write((uint8_t)((length >> 8) & 0xFF));
    _client.write((uint8_t)(length & 0xFF));
  } else {
    _client.write(0x80 | 127);
    for (int i = 7; i >= 0; i--) {
      _client.write((uint8_t)((length >> (i * 8)) & 0xFF));
    }
  }

  // Write mask
  _client.write(mask, 4);

  // Write masked payload
  for (size_t i = 0; i < length; i++) {
    _client.write((uint8_t)(payload[i] ^ mask[i % 4]));
  }

  return true;
}

void SimpleWebSocketClient::handlePing(const char *payload, size_t length) {
  // Respond with pong containing same payload
  sendFrame(WS_OPCODE_PONG, payload, length);
}

void SimpleWebSocketClient::handleClose() {
  Serial.println("[WS] Server sent close frame");
  _client.stop();
  _state = WS_DISCONNECTED;
}

String SimpleWebSocketClient::generateWebSocketKey() {
  // Generate 16 random bytes and base64 encode
  uint8_t bytes[16];
  for (int i = 0; i < 16; i++) {
    bytes[i] = random(256);
  }
  return base64Encode(bytes, 16);
}

String SimpleWebSocketClient::base64Encode(const uint8_t *data, size_t length) {
  static const char *b64chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  String result;
  result.reserve((length + 2) / 3 * 4);

  for (size_t i = 0; i < length; i += 3) {
    uint32_t n = ((uint32_t)data[i]) << 16;
    if (i + 1 < length)
      n |= ((uint32_t)data[i + 1]) << 8;
    if (i + 2 < length)
      n |= data[i + 2];

    result += b64chars[(n >> 18) & 0x3F];
    result += b64chars[(n >> 12) & 0x3F];
    result += (i + 1 < length) ? b64chars[(n >> 6) & 0x3F] : '=';
    result += (i + 2 < length) ? b64chars[n & 0x3F] : '=';
  }

  return result;
}

// ============ Socket.IO Protocol Handling ============
// Socket.IO Engine.IO packet types:
// 0 = open (server sends connection info)
// 1 = close
// 2 = ping
// 3 = pong
// 4 = message (Socket.IO layer)
// Socket.IO message types (after "4"):
// 0 = connect
// 2 = event  (so "42" = event message)

bool SimpleWebSocketClient::handleSocketIOMessage(const char *message,
                                                  size_t length) {
  if (length == 0)
    return false;

  char packetType = message[0];

  switch (packetType) {
  case '0': // Engine.IO open packet
    Serial.println("[SIO] Open packet received");
    // Send Socket.IO connect message
    sendSocketIOPacket("40");
    break;

  case '2': // Engine.IO ping
    Serial.println("[SIO] Ping received, sending pong");
    sendSocketIOPacket("3");
    break;

  case '3': // Engine.IO pong
    Serial.println("[SIO] Pong received");
    break;

  case '4': // Socket.IO message
    if (length > 1) {
      char messageType = message[1];

      if (messageType == '0') {
        // Socket.IO connect acknowledgment
        Serial.println("[SIO] Connected to Socket.IO server");
        _socketIOConnected = true;
      } else if (messageType == '2') {
        // Socket.IO event: 42["event_name", data]
        // Extract the JSON array after "42"
        const char *jsonStart = message + 2;
        size_t jsonLen = length - 2;

        if (jsonLen > 0 && _messageCallback) {
          // Pass the event data to the callback
          _messageCallback(jsonStart, jsonLen);
          return true;
        }
      }
    }
    break;

  default:
    Serial.print("[SIO] Unknown packet type: ");
    Serial.println(packetType);
    break;
  }

  return false;
}

void SimpleWebSocketClient::sendSocketIOPacket(const char *packet) {
  sendFrame(WS_OPCODE_TEXT, packet, strlen(packet));
}
