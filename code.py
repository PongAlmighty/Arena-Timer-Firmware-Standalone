import board
import busio
import time
import os
import displayio
import rgbmatrix
import framebufferio
from digitalio import DigitalInOut
from adafruit_esp32spi import adafruit_esp32spi
import adafruit_connection_manager

from timer import Timer
from timer_display import TimerDisplay
from web_server import WebServer
from socketio_client import SocketIOClient

# --- Hardware Setup ---
displayio.release_displays()

# Matrix Portal M4 Pinout for HUB75
matrix = rgbmatrix.RGBMatrix(
    width=64, height=32, bit_depth=1,
    rgb_pins=[board.MTX_R1, board.MTX_G1, board.MTX_B1, board.MTX_R2, board.MTX_G2, board.MTX_B2],
    addr_pins=[board.MTX_ADDRA, board.MTX_ADDRB, board.MTX_ADDRC, board.MTX_ADDRD],
    clock_pin=board.MTX_CLK, latch_pin=board.MTX_LAT, output_enable_pin=board.MTX_OE
)
display = framebufferio.FramebufferDisplay(matrix, auto_refresh=True)

# --- WiFi Setup (Airlift / ESP32 SPI) ---
print("Initializing WiFi (Airlift)...")
esp32_cs = DigitalInOut(board.ESP_CS)
esp32_ready = DigitalInOut(board.ESP_BUSY)
esp32_reset = DigitalInOut(board.ESP_RESET)
spi = busio.SPI(board.SCK, board.MOSI, board.MISO)
esp = adafruit_esp32spi.ESP_SPIcontrol(spi, esp32_cs, esp32_ready, esp32_reset)

if esp.status == adafruit_esp32spi.WL_IDLE_STATUS:
    print("ESP32 found and is idle")
print(f"Firmware vers: {esp.firmware_version}") # Use string or bytearray directly

print("Scanning for APs...")
try:
    for ap in esp.scan_networks():
        # Handle both dictionary and object return types for compatibility
        if isinstance(ap, dict):
             ssid = str(ap['ssid'], 'utf-8')
             rssi = ap['rssi']
        else:
             # Newer adafruit_esp32spi returns objects with .ssid (bytearray) and .rssi attributes
             ssid = str(ap.ssid, 'utf-8')
             rssi = ap.rssi
        print(f"\t{ssid} \t\tRSSI: {rssi}")
except Exception as e:
    print(f"Scan error (ignoring): {e}")

print("Connecting to WiFi...")
try:
    ssid = os.getenv("CIRCUITPY_WIFI_SSID")
    password = os.getenv("CIRCUITPY_WIFI_PASSWORD")
    if not ssid:
        print("Error: CIRCUITPY_WIFI_SSID not found in settings.toml")
    else:
        print(f"Attempting to connect to: {ssid}")
        # Retry loop
        for i in range(3):
            try:
                esp.connect_AP(ssid, password)
                print(f"Connected! IP: {esp.pretty_ip(esp.ip_address)}")
                break
            except RuntimeError as e:
                print(f"Connection attempt {i+1} failed: {e}")
                time.sleep(2)
                continue
except Exception as e:
    print(f"WiFi Connection failed: {e}")

# --- Initialize Objects ---
arena_timer = Timer()
timer_display = TimerDisplay(display)
timer_display.set_timer(arena_timer)

# Default Duration: 3 minutes
arena_timer.set_duration(3, 0)

# Pass the 'esp' object to WebServer and SocketIOClient
web_server = WebServer(arena_timer, timer_display, esp)
ws_client = SocketIOClient(arena_timer, esp)

# Setup Display Group
display.root_group = timer_display.group

# --- Main Loop ---
print("Arena Timer Running...")
last_web_poll = 0
last_ws_poll = 0

while True:
    # Update timer logic and display
    timer_display.update()
    
    # Poll Web Server
    now = time.monotonic()
    if now - last_web_poll > 0.05:
        web_server.poll()
        last_web_poll = now
        
    # Poll WebSocket (Socket.IO)
    if now - last_ws_poll > 0.1:
        ws_client.poll()
        last_ws_poll = now
        
    time.sleep(0.01)
