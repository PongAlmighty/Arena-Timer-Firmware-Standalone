import json
import adafruit_connection_manager
import binascii
import os

# Minimal standalone WebSocket / Socket.IO client for Matrix Portal M4 (ESP32SPI)
class SocketIOClient:
    def __init__(self, timer, esp):
        self._timer = timer
        self._esp = esp
        self._sock = None
        self._connected = False
        self._host = ""
        self._port = 8765
        self._path = "/socket.io/"

    def connect(self, host, port=8765, path="/socket.io/"):
        self._host = host
        self._port = port
        self._path = path
        
        try:
            print(f"Opening socket to {host}:{port}")
            # Get socket from connection manager using ESP32SPI
            self._sock = adafruit_connection_manager.get_connection_manager(self._esp).get_socket(host, port, "http:")
            
            # WebSocket Handshake
            key = binascii.b2a_base64(os.urandom(16)).decode("utf-8").strip()
            handshake = (
                f"GET {path}?EIO=4&transport=websocket HTTP/1.1\r\n"
                f"Host: {host}:{port}\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                f"Sec-WebSocket-Key: {key}\r\n"
                "Sec-WebSocket-Version: 13\r\n"
                "\r\n"
            )
            self._sock.send(handshake.encode("utf-8"))
            
            # Receive response
            resp = self._sock.recv(1024).decode("utf-8")
            if "101 Switching Protocols" in resp:
                print("WebSocket Connected (Handshake Successful)")
                self._connected = True
                self._sock.setblocking(False)
                return True
            else:
                print(f"Handshake failed: {resp}")
                self._sock.close()
                return False
        except Exception as e:
            print(f"Connection error: {e}")
            return False

    def disconnect(self):
        if self._sock:
            self._sock.close()
        self._connected = False
        self._sock = None

    @property
    def is_connected(self):
        return self._connected

    def poll(self):
        if not self._connected or not self._sock:
            return

        try:
            data = self._sock.recv(2048)
            if not data:
                return

            if len(data) < 2: return
            
            length = data[1] & 127
            offset = 2
            if length == 126:
                length = (data[2] << 8) | data[3]
                offset = 4
            elif length == 127:
                offset = 10
            
            if data[0] == 0x81:
                content = data[offset:offset+length].decode("utf-8")
                self._handle_message(content)
        except OSError:
            pass
        except Exception as e:
            print(f"Poll error: {e}")
            self._connected = False

    def _handle_message(self, data):
        if data.startswith("0"):
            self._sock.send(self._encode_frame("40"))
        elif data.startswith("2"):
            self._sock.send(self._encode_frame("3"))
        elif data.startswith("42"):
            try:
                msg = json.loads(data[2:])
                if isinstance(msg, list) and len(msg) >= 2:
                    if msg[0] == "timer_update":
                        self._handle_timer_update(msg[1])
            except:
                pass

    def _encode_frame(self, text):
        data = text.encode("utf-8")
        header = bytearray([0x81])
        length = len(data)
        if length <= 125:
            header.append(length)
        elif length <= 65535:
            header.append(126)
            header.append((length >> 8) & 0xFF)
            header.append(length & 0xFF)
        return header + data

    def _handle_timer_update(self, payload):
        action = payload.get("action")
        if action == "start":
            self._timer.start()
        elif action == "stop":
            self._timer.stop()
        elif action == "reset":
            m = payload.get("minutes", 3)
            s = payload.get("seconds", 0)
            self._timer.set_duration(m, s)
            self._timer.reset()
