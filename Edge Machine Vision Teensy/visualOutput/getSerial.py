import serial
import time
import viewRAW

PORT = 'COM3'
BAUD = 115200
START_MARKER = b'\xAA\x55\xAA\x55'
END_MARKER = b'\x55\xAA\x55\xAA'
WIDTH = 160
HEIGHT = 120
BYTES_PER_PIXEL = 2  # RGB565 = 2, Grayscale = 1

EXPECTED_SIZE = WIDTH * HEIGHT * BYTES_PER_PIXEL

filename = f"frame_{int(time.time())}.raw"

def read_frame(ser):
    print("Waiting for frame...")
    buffer = b''
    recording = False

    while True:
        byte = ser.read(1024) # Read in chunks
        if not byte:
            continue

        buffer += byte

        if not recording:
            if START_MARKER in buffer:
                buffer = buffer[buffer.find(START_MARKER) + len(START_MARKER):]
                recording = True
        else:
            if END_MARKER in buffer:
                buffer = buffer[:buffer.find(END_MARKER)]
                break
    
    if len(buffer) != EXPECTED_SIZE:
        print(f"Warning: Expected {EXPECTED_SIZE} bytes, got {len(buffer)}")
    return buffer

def save_raw_as_file(raw_data, filename):
    if len(raw_data) < EXPECTED_SIZE:
        raw_data += b'\x00' * (EXPECTED_SIZE - len(raw_data))  # pad with black
    with open(filename, 'wb') as f:
        f.write(raw_data)
    print(f"Saved raw data to {filename}")

def main():
    with serial.Serial(PORT, BAUD, timeout=10) as ser:
        frame = read_frame(ser)
        save_raw_as_file(frame, filename)
        viewRAW.printing(filename)

if __name__ == "__main__":
    main()
