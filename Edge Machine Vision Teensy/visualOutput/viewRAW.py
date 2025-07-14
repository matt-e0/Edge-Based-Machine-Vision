from PIL import Image

WIDTH = 160
HEIGHT = 120

def printing(raw):

    # Convert RGB565 to RGB888
    def rgb565_to_rgb888(raw):
        rgb_data = bytearray()
        for i in range(0, len(raw), 2):
            pixel = (raw[i] << 8) | raw[i+1]
            r = (pixel >> 11) & 0x1F
            g = (pixel >> 5) & 0x3F
            b = pixel & 0x1F
            rgb_data += bytes([
                int(r * 255 / 31),
                int(g * 255 / 63),
                int(b * 255 / 31)
            ])
        return bytes(rgb_data)


    #rgb888 = rgb565_to_rgb888(raw)
    rgb888 = raw

    print(f"Expected size: {WIDTH * HEIGHT * 3}")
    print(f"Actual rgb888 size: {len(rgb888)}") # 54540 bytes

    img = Image.frombytes("RGB", (WIDTH, HEIGHT), rgb888)
    img.save(str("test")+ ".png")
    img.show()

