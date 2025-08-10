#include <stdint.h>
#include <vector>

extern int blobThreshold; // Maximum deviation for blob tracking (pixels)

struct Blob {
    int id;
    int minX, maxX, minY, maxY;
    int sumX, sumY;
    int pixelCount;
    float centreX, centreY;
};

struct Pixel { 
    int x, y; 
};

struct TrackerState {
    int lastCentroidX = -1;
    int lastCentroidY = -1;
    int lastPixelCount = 0;
};

inline bool getPixelMask(int x, int y, const uint8_t mask[], int pixelWidth) {
  int bitIndex = y * pixelWidth + x;
  int byteIndex = bitIndex / 8;
  int bitOffset = bitIndex % 8;
  return (mask[byteIndex] >> bitOffset) & 1;
}

inline void clearPixelMask(int x, int y, uint8_t mask[], int pixelWidth) {
    int bitIndex  = y * pixelWidth + x;
    int byteIndex = bitIndex >> 3;
    int bitOffset = bitIndex & 7;
    mask[byteIndex] &= ~(1 << bitOffset); // Set bit to 0
}

void detectBlobs(int pixelHeight, int pixelWidth, uint8_t mask[], std::vector<Blob> &blobs);

void setCurrentTarget(std::vector<Blob> &blobs, bool &targetSet, TrackerState &state);



Pixel trackBlob(const std::vector<Blob> &blobs, int blobThreshold, TrackerState &state);
