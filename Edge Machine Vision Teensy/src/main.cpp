#include <Arduino.h>
#include <Wire.h>
#include <string>
#include <SPI.h>
#include <ArduCAM.h>
#include <memorysaver.h>
#include <Servo.h>
#include <blobDetection.h>
#include <camera.h>

#if !(defined (OV2640_MINI_2MP_PLUS))
#error Enable OV2640_MINI_2MP_PLUS in memorysaver.h
#endif

#define CS_PIN 10

// Camera module setup
ArduCAM myCAM(OV2640, CS_PIN);
uint8_t mask[bitmaskSize]; // 1D bit array 

// Debug
//const uint32_t expectedLength = (pixelWidth * pixelHeight * 2) + 8;

// Blob
TrackerState tracker;
std::vector<Blob> blobs;
bool targetSet = false;
int persistanceFrames = 3;

// Servo paramters
Servo SERVOH;
Servo SERVOV;

const int centerX = pixelWidth / 2;
const int centerY = pixelHeight / 2;
const int deadzone = 5;
const float KpH = 0.1;
const float KpV = 0.1;
const bool invertX = true; 
const bool invertY = false; 
// Start position
static int servoHPos = 90;
static int servoVPos = 90;


void trackServo(Pixel p) {
  if (p.x >= 0 && p.y >= 0) {
      int errorX = p.x - centerX;
      int errorY = p.y - centerY;

      if (invertX) errorX = -errorX;
      if (invertY) errorY = -errorY;

      // Check for deviation
      if (abs(errorX) > deadzone) {
          servoHPos += errorX * KpH;
          servoHPos = constrain(servoHPos, 0, 180);
          SERVOH.write(servoHPos);
      }

      if (abs(errorY) > deadzone) {
          servoVPos += errorY * KpV;
          servoVPos = constrain(servoVPos, 0, 180);
          SERVOV.write(servoVPos);
      }
    }
}

inline void setPixelMask(int x, int y, bool value) {
  int bitIndex = y * pixelWidth + x;
  int byteIndex = bitIndex / 8;
  int bitOffset = bitIndex % 8;

  if (value)
    mask[byteIndex] |= (1 << bitOffset);
  else
    mask[byteIndex] &= ~(1 << bitOffset);
}


bool isTargetColour(uint16_t rgb565) {
  uint8_t r = (rgb565 >> 11) & 0x1F;
  uint8_t g = (rgb565 >> 5) & 0x3F;
  uint8_t b = rgb565 & 0x1F;

  r = (r * 255) / 31;
  g = (g * 255) / 63;
  b = (b * 255) / 31;

  if ((r > 110 && r < 255) &&
      (g > 0 && g < 60) &&
      (b > 0 && b < 90)&&
      (abs(g - b) < 30)) {
    return true;
  }
  return false;
}

void sendRGB565() {
  initializeFrame();
  // Read image pixel by pixel 
  for (int y = 0; y < pixelHeight; y++) {
    for (int x = 0; x < pixelWidth; x++) {
      // rgb565 format is 2 bytes long
      // The HI byte is the most important, containing 5 red bits and 3 green bits, LO 3 green and 5 blue
      // Each pixel is read byte by byte
      uint8_t high = SPI.transfer(0x00);
      uint8_t low = SPI.transfer(0x00);
      uint16_t pixel565 = (high << 8) | low;

      setPixelMask(x, y, isTargetColour(pixel565));
      
      readBytes(low, high);
    }
  }

  flushBuffer();

  endFrame(); 
  
  myCAM.CS_HIGH(); 
  //Serial.println("\nImage sent!");
  yield();
}


void captureFrameWithThreshold() {

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
  //Serial.println("Capturing...");

  // Wait until capture is done
  uint32_t startTime = millis();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK)) {
    if (millis() - startTime > 2000) {
      //Serial.println("Capture timeout.");
      return;
    }
  }

  //Serial.println("Capture done!");

  /* DEBUG
  uint32_t length = myCAM.read_fifo_length();
  uint32_t start = micros();
// Code section
Serial.printf("Time: %lu us\n", micros() - start);
  
Bulk transfer serial instead of using serial.write()
Use fixed-point math instead of floats where possible
check build flags
Use CMSIS-DSP library for fast convolution, FFT, filtering.
Use DMAMEM for large frame buffers to keep them off the tightly coupled RAM.
Ensure arrays are static or global to avoid stack overflows
Find a way to perform parallelism/pipelining
THink of cropping the image after lock?
RTOS?


  if (length != expectedLength) {
    Serial.print("Unexpected image length: ");
    Serial.println(length);
    Serial.println(expectedLength);
    return;
  }
  */

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  sendRGB565();
  //printMask(); // Needed for getMask.py, can be commented out
  delay(20);  // Optional pacing
}


void setup() {
  uint8_t vid, pid;
  uint8_t temp;

  SERVOH.attach(15);
  SERVOV.attach(14);

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
  if ((vid != 0x26) || ((pid != 0x42) && (pid != 0x41))) {
    Serial.println("Can't find OV2640 module!");
    while (1);
  } else {
    Serial.println("OV2640 detected.");
  }

  // Initialize camera
  myCAM.set_format(BMP);
  myCAM.InitCAM();
  //myCAM.OV2640_set_Brightness(Brightness2);
  //myCAM.OV2640_set_Color_Saturation(2);
  //myCAM.OV2640_set_Light_Mode(Auto);
  //myCAM.OV2640_set_Contrast(1);
  myCAM.OV2640_set_JPEG_size(OV2640_160x120);
  myCAM.clear_fifo_flag();

  delay(10);
}

void loop() {
  //delay(2000);
  if (!targetSet) {
        captureFrameWithThreshold();
        detectBlobs(pixelHeight, pixelWidth, mask, blobs);
        setCurrentTarget(blobs, targetSet, tracker);

        if (targetSet) {
            persistanceFrames = 3;
        }
    } 
    else {
        captureFrameWithThreshold();
        detectBlobs(pixelHeight, pixelWidth, mask, blobs);

        Pixel p = trackBlob(blobs, blobThreshold, tracker);

        //Serial.print("X:");
        //Serial.print(p.x);
        //Serial.print(" Y:");
        //Serial.println(p.y);

        if (p.x == -1 && p.y == -1) {
            persistanceFrames--;
            if (persistanceFrames > 0) {
                captureFrameWithThreshold();
                detectBlobs(pixelHeight, pixelWidth, mask, blobs);
                setCurrentTarget(blobs, targetSet, tracker);
            } else {
                targetSet = false;
            }
        } 
        else {
            yield();
            trackServo(p);
            persistanceFrames = 5; // Reset if tracking successful
        }
    }
}