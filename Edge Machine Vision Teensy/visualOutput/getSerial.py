import serial
import time
import viewRAW
import cv2
import numpy as np

PORT = 'COM3'
BAUD = 115200
START_MARKER = b'\xAA\x55\xAA\x55'
END_MARKER = b'\x55\xAA\x55\xAA'
WIDTH = 160
HEIGHT = 120
BYTES_PER_PIXEL = 2  # RGB565
EXPECTED_SIZE = WIDTH * HEIGHT * BYTES_PER_PIXEL

def rgb565_to_rgb888(rgb565_bytes):
    if len(rgb565_bytes) != 2:
        raise ValueError("Input must be exactly 2 bytes")

    value = (rgb565_bytes[0] << 8) | rgb565_bytes[1]

    r = (value >> 11) & 0x1F
    g = (value >> 5) & 0x3F
    b = value & 0x1F

    r = (r << 3) | (r >> 2)
    g = (g << 2) | (g >> 4)
    b = (b << 3) | (b >> 2)

    return (r, g, b)

def read_frame(ser):
    print("Waiting for frame...")
    buffer = b''
    while True:
        chunk = ser.read(1024)
        if not chunk:
            continue
        buffer += chunk

        if START_MARKER in buffer and END_MARKER in buffer:
            start_index = buffer.find(START_MARKER)
            end_index = buffer.find(END_MARKER)

            if end_index < start_index:
                print("Malformed frame (END before START)")
                buffer = buffer[end_index + len(END_MARKER):]  # Drop garbage
                continue

            frame_data = buffer[start_index + len(START_MARKER):end_index]

            # Clean up buffer to remove used data
            buffer = buffer[end_index + len(END_MARKER):]

            break

    if len(frame_data) > EXPECTED_SIZE:
        print(f"⚠️ Trimming extra {len(frame_data) - EXPECTED_SIZE} bytes from frame_data")
        frame_data = frame_data[:EXPECTED_SIZE]
    elif len(frame_data) < EXPECTED_SIZE:
        print(f"⚠️ Warning: Frame too short ({len(frame_data)} bytes), skipping")
        return None


    frame_rgb888 = []
    for i in range(0, len(frame_data), 2):
        pixel_bytes = frame_data[i:i+2]
        rgb888 = rgb565_to_rgb888(pixel_bytes)
        frame_rgb888.append(rgb888)

    return frame_rgb888


def main():
    with serial.Serial(PORT, BAUD, timeout=10) as ser:
        while True:
            frame_rgb888 = read_frame(ser)
            if frame_rgb888 is None:
                continue

            # Convert list of tuples to NumPy array
            frame_array = np.array(frame_rgb888, dtype=np.uint8).reshape((HEIGHT, WIDTH, 3))
            cv2.imshow('Live Frame', frame_array)
            #viewRAW.printing(frame_rgb888)

            # Exit on ESC key
            if cv2.waitKey(1) & 0xFF == 27:
                break

        cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
