import serial
import time

def monitor_serial(port, duration=10):
    print(f"Monitoring {port} for {duration} seconds...")
    try:
        with serial.Serial(port, 115200, timeout=1) as ser:
            start_time = time.time()
            all_data = b""
            while time.time() - start_time < duration:
                if ser.in_waiting:
                    data = ser.read(ser.in_waiting)
                    all_data += data
                    print(data.decode('utf-8', errors='ignore'), end='', flush=True)
                time.sleep(0.1)
            
            print("\nMonitoring finished.")
            with open("serial_dump.txt", "w") as f:
                f.write(all_data.decode('utf-8', errors='ignore'))
                
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    monitor_serial('/dev/cu.usbmodem314101')
