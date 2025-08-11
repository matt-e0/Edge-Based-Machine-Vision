#include <camera.h>
#include <Arduino.h>
#include <blobDetection.h>

// Buffer
// (Buffer is chunkSize (64) bytes long)
const int chunkSize = 64; 
uint8_t sendBuffer[chunkSize];
int bufIndex = 0;

// For live view using getMask.py
void printMask() {
  for (int y = 0; y < pixelHeight; y++) {
    for (int x = 0; x < pixelWidth; x++) {
      Serial.print(getPixelMask(x, y, mask, pixelWidth) ? "1" : "0");
    }
    Serial.println();
  }
}

// Beginning of frame byte marker
void initializeFrame() {
  if (serialOut) {
      while (Serial.availableForWrite() < 2) yield();
      Serial.write(startByte, sizeof(startByte));
  }
}

// End of frame byte marker
void endFrame() {
  if (serialOut) {
    while (Serial.availableForWrite() < 2) yield();
    Serial.write(endByte, sizeof(endByte));
    Serial.flush();  // Waits until all data is sent
  }
}

void readBytes(uint8_t low, uint8_t high) {
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

void flushBuffer() {
  if (bufIndex > 0 && serialOut) {
    while (Serial.availableForWrite() < bufIndex) yield();
    Serial.write(sendBuffer, bufIndex);
  }
}