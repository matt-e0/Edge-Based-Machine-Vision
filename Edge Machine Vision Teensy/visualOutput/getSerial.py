import serial

PORT = 'COM3'
BAUD = 115200
JPEG_START = b'\xFF\xD8'
JPEG_END = b'\xFF\xD9'

ser = serial.Serial(PORT, BAUD, timeout=5)

def read_jpeg(ser):
    print("Waiting for JPEG data...")
    data = b''
    recording = False

    while True:
        byte = ser.read()
        if not byte:
            break
        data += byte
        if not recording:
            if JPEG_START in data:
                data = data[data.find(JPEG_START):]
                recording = True
        elif JPEG_END in data:
            data = data[:data.find(JPEG_END)+2]
            break

    return data

image = read_jpeg(ser)
with open("output.jpg", "wb") as f:
    f.write(image)
print("Image saved to output.jpg")
