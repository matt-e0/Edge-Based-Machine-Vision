import serial
import pygame
import sys


SERIAL_PORT = 'COM3'
BAUD_RATE = 921600

WIDTH, HEIGHT = 160, 120
PIXEL_SIZE = 4  # scale factor for display window

def main():
    pygame.init()
    screen = pygame.display.set_mode((WIDTH * PIXEL_SIZE, HEIGHT * PIXEL_SIZE))
    pygame.display.set_caption("Camera Mask Display")

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    except Exception as e:
        print(f"Failed to open serial port {SERIAL_PORT}: {e}")
        sys.exit(1)

    print(f"Listening on {SERIAL_PORT} at {BAUD_RATE} baud...")

    # Buffer to store mask
    mask_lines = []

    running = True
    while running:
        # Handle pygame events to close window
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                running = False

        try:
            line = ser.readline().decode('ascii', errors='ignore').strip()
        except Exception as e:
            print(f"Serial read error: {e}")
            continue

        # Check if line length matches the mask width
        if len(line) == WIDTH and all(c in '01' for c in line): # Check bitstream is 0,1s
            mask_lines.append(line)

            # When we get full frame, draw it
            if len(mask_lines) == HEIGHT:
                for y, row in enumerate(mask_lines):
                    for x, c in enumerate(row):
                        color = (255, 255, 255) if c == '1' else (0, 0, 0)
                        rect = pygame.Rect(x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE)
                        pygame.draw.rect(screen, color, rect)

                pygame.display.flip()
                mask_lines = []

    ser.close()
    pygame.quit()

if __name__ == '__main__':
    main()