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

constexpr int bitmaskSize = (pixelWidth * pixelHeight + 7) / 8;
uint8_t mask[bitmaskSize]; // 1D bit array

const uint8_t startByte[] = { 0xAA, 0x55, 0xAA, 0x55 };
const uint8_t endByte[]   = { 0x55, 0xAA, 0x55, 0xAA };

const size_t chunkSize = 64; 
uint8_t sendBuffer[chunkSize];
size_t bufIndex = 0;
bool serialOut = false;

inline void setPixelMask(int x, int y, bool value) {
  int bitIndex = y * pixelWidth + x;
  int byteIndex = bitIndex / 8;
  int bitOffset = bitIndex % 8;

  if (value)
    mask[byteIndex] |= (1 << bitOffset);
  else
    mask[byteIndex] &= ~(1 << bitOffset);
}

inline bool getPixelMask(int x, int y) {
  int bitIndex = y * pixelWidth + x;
  int byteIndex = bitIndex / 8;
  int bitOffset = bitIndex % 8;
  return (mask[byteIndex] >> bitOffset) & 1;
}

void printMask() {
  for (int y = 0; y < pixelHeight; y++) {
    for (int x = 0; x < pixelWidth; x++) {
      Serial.print(getPixelMask(x, y) ? "1" : "0");
    }
    Serial.println();
  }
}

bool isTargetColour(uint16_t rgb565) {
  uint8_t r = (rgb565 >> 11) & 0x1F;
  uint8_t g = (rgb565 >> 5) & 0x3F;
  uint8_t b = rgb565 & 0x1F;

  r = (r * 255) / 31;
  g = (g * 255) / 63;
  b = (b * 255) / 31;

  if (r > 100 && r < 255 && g < 50 && b < 50) {
    return true;
  }
  return false;
}

void sendRGB565() {
  if (serialOut) {
    while (Serial.availableForWrite() < 2) yield();
    Serial.write(startByte, sizeof(startByte));
  }

  // Read, threshold, and send
  for (int y = 0; y < pixelHeight; y++) {
    for (int x = 0; x < pixelWidth; x++) {
      uint8_t high = SPI.transfer(0x00);
      uint8_t low = SPI.transfer(0x00);
      uint16_t pixel565 = (high << 8) | low;

      setPixelMask(x, y, isTargetColour(pixel565));

      if (serialOut) {
        sendBuffer[bufIndex++] = low;
        sendBuffer[bufIndex++] = high;

        if (bufIndex >= chunkSize) {
          while (Serial.availableForWrite() < bufIndex) yield();
          Serial.write(sendBuffer, bufIndex);
          bufIndex = 0;
        }
      }

    }
  }

  if (bufIndex > 0 && serialOut) {
    while (Serial.availableForWrite() < bufIndex) yield();
    Serial.write(sendBuffer, bufIndex);
  }

  if (serialOut) {
    while (Serial.availableForWrite() < 2) yield();
    Serial.write(endByte, sizeof(endByte));
  }
  
  myCAM.CS_HIGH();
  Serial.flush();  // Waits until all data is sent

  Serial.println("\nImage sent!");
  yield();
}


void captureFrameWithThreshold() {

  while (Serial.availableForWrite() < 2) yield();
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

  uint32_t length = myCAM.read_fifo_length();
  const uint32_t expectedLength = (pixelWidth * pixelHeight * 2) + 8;

  if (length != expectedLength) {
    Serial.print("Unexpected image length: ");
    Serial.println(length);
    return;
  }

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  sendRGB565();
  printMask();
  delay(50);  // Optional pacing
}


void setup() {
  uint8_t vid, pid;
  uint8_t temp;

  Wire.begin();
  Serial.begin(921600);
  delay(100);
  
  Serial.println("Camera start");

  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);

  SPI.begin();
  delay(50);

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
    delay(100);
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

  delay(50);
}

void loop() {
  captureFrameWithThreshold();
}
