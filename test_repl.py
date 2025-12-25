import serial
import time

def enter_repl(port):
    print(f"Attempting to enter REPL on {port}")
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            # Send multiple Ctrl-C to break any running script
            for _ in range(5):
                ser.write(b'\x03')
                time.sleep(0.1)
            
            time.sleep(0.5)
            # Send Enter
            ser.write(b'\r\n')
            time.sleep(0.5)
            
            output = ser.read(2000).decode('utf-8', errors='ignore')
            print("Output after Ctrl-C:")
            print(output)
            
            if ">>>" in output or "Adafruit" in output or "CircuitPython" in output:
                print("Successfully in REPL!")
            else:
                print("Still not in REPL. Checking if it's responding at all.")
                ser.write(b'print("HELLO")\r\n')
                time.sleep(0.5)
                output = ser.read(2000).decode('utf-8', errors='ignore')
                print("Output after 'print':")
                print(output)
                
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    enter_repl('/dev/cu.usbmodem314101')
