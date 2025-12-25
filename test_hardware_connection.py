import serial
import time

def test_serial(port):
    print(f"Testing port: {port}")
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            print(f"Successfully opened {port}")
            # Send a Ctrl-C to break out of any running script if it's CircuitPython
            ser.write(b'\x03')
            time.sleep(0.5)
            # Send a Ctrl-D to soft reboot or just see output
            ser.write(b'\x04')
            time.sleep(1)
            output = ser.read(1000).decode('utf-8', errors='ignore')
            print("Received output:")
            print(output)
            
            # Send 'help()' or something simple
            ser.write(b'help()\r\n')
            time.sleep(1)
            output = ser.read(1000).decode('utf-8', errors='ignore')
            print("Received response to help():")
            print(output)
            
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    test_serial('/dev/cu.usbmodem314101')
