#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduCAM.h>
#include <memorysaver.h>

#if !(defined (OV2640_MINI_2MP_PLUS))
#error Enable OV2640_MINI_2MP_PLUS in memorysaver.h
#endif

#define CS_PIN 10

ArduCAM myCAM(OV2640, CS_PIN);

void setup() {
  uint8_t vid, pid;
  uint8_t temp;

  Wire.begin();
  Serial.begin(115200);
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
  delay(1000);
}

void loop() {
  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();

  // Start capture
  myCAM.start_capture();
  Serial.println("Capturing...");

  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));

  Serial.println("Capture done!");

  // Read image length
  uint32_t length = myCAM.read_fifo_length();
  if (length >= 0x7FFFFF) {
    Serial.println("Image too large.");
    return;
  }

  if (length == 0) {
    Serial.println("No image captured.");
    return;
  }

  Serial.print("Image length: ");
  Serial.println(length);

  // Transfer image via serial
  Serial.println("Sending image:");
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  const size_t chunkSize = 4096;
  uint8_t buf[chunkSize];
  uint32_t bytes_read = 0;

  const uint8_t START_MARKER[] = { 0xAA, 0x55, 0xAA, 0x55 };
  const uint8_t END_MARKER[]   = { 0x55, 0xAA, 0x55, 0xAA };

  // After capture:
  Serial.write(START_MARKER, sizeof(START_MARKER));

  while (bytes_read < length) {
    size_t bytes_to_read = min(chunkSize, length - bytes_read);
    SPI.transfer(buf, bytes_to_read);
    Serial.write(buf, bytes_to_read);
    bytes_read += bytes_to_read;
  }

  Serial.write(END_MARKER, sizeof(END_MARKER));

  myCAM.CS_HIGH();
  Serial.println("\nImage sent!");
  delay(5000); // Wait 5 seconds between captures
}
