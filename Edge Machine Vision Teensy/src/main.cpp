#include <Arduino.h>
#include <Wire.h>
#include <string>
#include <SPI.h>
#include <ArduCAM.h>
#include <memorysaver.h>

#if !(defined (OV2640_MINI_2MP_PLUS))
#error Enable OV2640_MINI_2MP_PLUS in memorysaver.h
#endif

#define CS_PIN 10

ArduCAM myCAM(OV2640, CS_PIN);
constexpr uint8_t pixelWidth = 160;
constexpr uint8_t pixelHeight = 120;

void printMask(uint8_t mask[pixelHeight][pixelWidth]) {
  for (int i = 0; i < pixelHeight; i++) {
    String line = "";
    for (int j = 0; j < pixelWidth; j++) {
      line += String(mask[i][j]);
    }
    Serial.println(line);
  }
}

bool isTargetColour(uint16_t rgb565) {
  uint8_t r = (rgb565 >> 11) & 0x1F;
  uint8_t g = (rgb565 >> 5) & 0x3F;
  uint8_t b = rgb565 & 0x1F;

  // Convert to 8-bit scale (approximate)
  r = (r * 255) / 31;
  g = (g * 255) / 63;
  b = (b * 255) / 31;

  // Define a loose threshold around your red target (e.g. #bd1020)
  if (r > 150 && r < 220 && g < 50 && b < 60) {
    return true;
  }
  return false;
}

void captureFrameWithThreshold() {
  uint8_t mask[pixelHeight][pixelWidth];

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
  Serial.println("Capturing...");

  // Wait until capture is done
  uint32_t startTime = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    if (millis() - startTime > 2000) {
      Serial.println("Capture timeout.");
      return;
    }
  }

  Serial.println("Capture done!");

  // Get image length
  uint32_t length = myCAM.read_fifo_length();
  const uint32_t expectedLength = (pixelWidth * pixelHeight * 2) + 8;

  if (length != expectedLength) {
    Serial.print("Unexpected image length: ");
    Serial.println(length);
    return;
  }

  // Allocate buffer for image
  uint8_t imageBuf[expectedLength];

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  // Read entire image to buffer
  for (uint32_t i = 0; i < expectedLength; i++) {
    imageBuf[i] = SPI.transfer(0x00);
  }

  myCAM.CS_HIGH();

  // Process thresholding from buffer
  for (int y = 0; y < pixelHeight; y++) {
    for (int x = 0; x < pixelWidth; x++) {
      size_t index = (y * pixelWidth + x) * 2;
      uint16_t pixel565 = (imageBuf[index] << 8) | imageBuf[index + 1];
      mask[y][x] = isTargetColour(pixel565) ? 1 : 0;
    }
  }

  // Send frame over serial
  const uint8_t START_MARKER[] = { 0xAA, 0x55, 0xAA, 0x55 };
  const uint8_t END_MARKER[]   = { 0x55, 0xAA, 0x55, 0xAA };

  Serial.write(START_MARKER, sizeof(START_MARKER));
  Serial.write(imageBuf, expectedLength);
  Serial.write(END_MARKER, sizeof(END_MARKER));

  Serial.println("\nImage sent!");
  delay(100); // Delay between frames
}


void setup() {
  uint8_t vid, pid;
  uint8_t temp;

  Wire.begin();
  Serial.begin(921600);
  delay(1000);
  Serial.println("Camera start");

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  SPI.begin();
  delay(1000);

  // Reset camera
  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);

  // Test SPI
  while (true) {
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp == 0x55) break;
    Serial.println("SPI interface error");
    delay(1000);
  }

  // Check if the camera module type is OV2640
  myCAM.rdSensorReg8_8(0x0A, &vid);
  myCAM.rdSensorReg8_8(0x0B, &pid);
  Serial.print("VID: 0x"); Serial.println(vid, HEX);
  Serial.print("PID: 0x"); Serial.println(pid, HEX);
  myCAM.wrSensorReg8_8(0xff, 0x01);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26) || (pid != 0x42) && (pid != 0x41)) {
    Serial.println("Can't find OV2640 module!");
    while (1);
  } else {
    Serial.println("OV2640 detected.");
  }

  // Initialize camera
  myCAM.set_format(BMP);
  myCAM.InitCAM();
  myCAM.OV2640_set_JPEG_size(OV2640_160x120);
  myCAM.clear_fifo_flag();

  delay(1000);
}

void loop() {
  captureFrameWithThreshold();
  delay(10);
}
