#include <stdint.h>

constexpr bool serialOut = false;

// Camera resolution
constexpr uint16_t pixelWidth = 160;
constexpr uint16_t pixelHeight = 120;

// Image mask
constexpr int bitmaskSize = (pixelWidth * pixelHeight + 7) / 8;
extern uint8_t mask[bitmaskSize]; // 1D bit array 

// If frames need to be sent via serial
const uint8_t startByte[] = { 0xAA, 0x55, 0xAA, 0x55 };
const uint8_t endByte[]   = { 0x55, 0xAA, 0x55, 0xAA };


void printMask();
void initializeFrame();
void endFrame();
void readBytes(uint8_t low, uint8_t high);
void flushBuffer();