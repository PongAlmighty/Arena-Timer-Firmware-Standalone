import serial
import time
import sys

def monitor(port):
    print(f"Opening {port}...")
    try:
        # Open with a short timeout, and enable DTR/RTS which can help trigger some boards
        ser = serial.Serial(port, 115200, timeout=0.1)
        ser.dtr = True
        ser.rts = True
        print("Monitoring... (Press Ctrl+C to stop)")
        
        while True:
            if ser.in_waiting:
                data = ser.read(ser.in_waiting)
                print(data.decode('utf-8', errors='ignore'), end='', flush=True)
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("\nStopped.")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    monitor('/dev/cu.usbmodem314101')
