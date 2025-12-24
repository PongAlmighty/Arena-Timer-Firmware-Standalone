import board
import busio
from digitalio import DigitalInOut
from adafruit_esp32spi import adafruit_esp32spi

print("Inspecting adafruit_esp32spi...")
print(f"Dir of adafruit_esp32spi: {dir(adafruit_esp32spi)}")

try:
    import adafruit_esp32spi.socket
    print("Found adafruit_esp32spi.socket")
    print(dir(adafruit_esp32spi.socket))
except ImportError:
    print("adafruit_esp32spi.socket NOT found")

try:
    from adafruit_esp32spi import socket
    print("Found socket in adafruit_esp32spi")
except ImportError:
    print("socket NOT in adafruit_esp32spi")

# Check connection manager pool type
import adafruit_connection_manager
esp32_cs = DigitalInOut(board.ESP_CS)
esp32_ready = DigitalInOut(board.ESP_BUSY)
esp32_reset = DigitalInOut(board.ESP_RESET)
spi = busio.SPI(board.SCK, board.MOSI, board.MISO)
esp = adafruit_esp32spi.ESP_SPIcontrol(spi, esp32_cs, esp32_ready, esp32_reset)
pool = adafruit_connection_manager.get_radio_socketpool(esp)
print(f"Pool type: {type(pool)}")
print(f"Pool dir: {dir(pool)}")

try:
    sock = pool.socket()
    print(f"Socket type: {type(sock)}")
    print(f"Socket dir: {dir(sock)}")
except Exception as e:
    print(f"Socket creation failed: {e}")
