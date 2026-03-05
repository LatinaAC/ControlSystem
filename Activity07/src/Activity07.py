import serial
import time
import sys

# Change 'COM3' to the correct port for your system
PORT = 'COM5'
BAUD_RATE = 9600

def connect_to_arduino():
    try:
        ser = serial.Serial(PORT, BAUD_RATE, timeout=1)
        # Give the connection time to initialize
        time.sleep(2)
        return ser
    except serial.SerialException as e:
        print(f"Could not open serial port {PORT}: {e}")
        sys.exit(1)

def main():
    ser = connect_to_arduino()
    print(f"Connected to Arduino on {PORT}")
    print("Enter an angle between 0 and 180 (or 'exit' to quit)")

    try:
        while True:
            user_input = input("\nAngle: ").strip()
            
            if user_input.lower() == 'exit':
                break
                
            try:
                angle = int(user_input)
                
                if 0 <= angle <= 180:
                    # Send angle as string followed by a newline
                    ser.write(f"{angle}\n".encode())
                    print(f"Angle {angle} sent to Arduino.")
                else:
                    print("Validation Error: Angle must be between 0 and 180.")
                    
            except ValueError:
                print("Input Error: Please enter a valid whole number.")
                
    except KeyboardInterrupt:
        print("\nProgram stopped by user.")
    finally:
        ser.close()
        print("Serial connection closed.")

if __name__ == "__main__":
    main()