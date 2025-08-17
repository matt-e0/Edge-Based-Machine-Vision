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

inline bool getPixelMask(int x, int y, const uint8_t* mask, int pixelWidth) {
  int idx = y * pixelWidth + x; // Linear index
  return (mask[idx >> 3] >> (idx & 7)) & 1; // idx>>3 gets the byte and idx&7 getss the bit from that byte
}

inline void clearPixelMask(int x, int y, uint8_t* mask, int pixelWidth) {
    int idx = y * pixelWidth + x; // Linear index
    mask[idx >> 3] &= ~(1 << (idx & 7));
}

void detectBlobs(int pixelHeight, int pixelWidth, uint8_t mask[], std::vector<Blob> &blobs);

void setCurrentTarget(std::vector<Blob> &blobs, bool &targetSet, TrackerState &state);



Pixel trackBlob(const std::vector<Blob> &blobs, int blobThreshold, TrackerState &state);
