#include "blobDetection.h"
#include <stdint.h>
#include <cmath>
// PLAN
/*
Check through each pixel in the mask to see if has been assigned to a blob
if not perform a BFS O(N) instead of checking every pixel again O(n^2)
*/

/*
Target identification can initialy be achieved through a "roundness" check or
checking motion compared to other blobs. The most ciritical check to determine if
it is the target blob will be the deviation of the centroid per frame.
*/

/*
Consider lock, predictive tracking and persistance of tracking when lock is lost
*/

int lastCentroidX = -1;
int lastCentroidY = -1;
int tempID = 0;
int lastPixelCount = 0;
int pixelCountThreshold = 6;
float circleThreshold = 1.4;
int blobThreshold = 15;


Pixel trackBlob(const std::vector<Blob> &blobs, int blobThreshold, TrackerState &state) {
    for (const Blob &target : blobs) {
        // Size match check
        if (target.pixelCount > state.lastPixelCount - pixelCountThreshold &&
            target.pixelCount < state.lastPixelCount + pixelCountThreshold) {

            int currentX = std::round(target.centreX);
            int currentY = std::round(target.centreY);

            // Position match check
            if (currentX > state.lastCentroidX - blobThreshold &&
                currentX < state.lastCentroidX + blobThreshold &&
                currentY > state.lastCentroidY - blobThreshold &&
                currentY < state.lastCentroidY + blobThreshold) {

                // Update tracker state
                state.lastCentroidX = currentX;
                state.lastCentroidY = currentY;
                state.lastPixelCount = target.pixelCount;
                return { currentX, currentY };
            }
        }
    }

    // If no match, clear tracking
    state.lastCentroidX = -1;
    state.lastCentroidY = -1;
    state.lastPixelCount = 0;
    return { -1, -1 };
}

void setCurrentTarget(std::vector<Blob> &blobs, bool &targetSet, TrackerState &state) {
    targetSet = false;
    for (Blob &target : blobs) {
        if (target.pixelCount > 12 &&
            target.sumY != 0 && target.sumX != 0 &&
            (float(target.sumX) / target.sumY < circleThreshold) &&
            (float(target.sumY) / target.sumX < circleThreshold)) {

            targetSet = true;
            state.lastPixelCount = target.pixelCount;
            state.lastCentroidX = std::round(target.centreX);
            state.lastCentroidY = std::round(target.centreY);
            break;
        }
    }
}


void detectBlobs(int pixelHeight, int pixelWidth, uint8_t mask[], std::vector<Blob> &blobs) {
    int persistanceFrames = 3;
    blobs.clear();
    std::vector<Pixel> queue; // BFS queue
    queue.reserve(pixelWidth * pixelHeight);

    for (int y = 0; y < pixelHeight; y++) {
        for (int x = 0; x < pixelWidth; x++) {
            if (getPixelMask(x, y, mask, pixelWidth)) {
                // Start Blob
                tempID += 1;
                Blob blob;
                blob.id = tempID;
                blob.minX = blob.maxX = x;
                blob.minY = blob.maxY = y;
                blob.sumX = blob.sumY = 0;
                blob.pixelCount = 0;

                queue.clear();
                queue.push_back({x, y});
                clearPixelMask(x, y, mask, pixelWidth);

                while (!queue.empty()) {
                    Pixel p = queue.back();
                    queue.pop_back();

                    // Update blob statistics
                    blob.sumX += p.x;
                    blob.sumY += p.y;
                    blob.pixelCount++;
                    if (p.x < blob.minX) blob.minX = p.x;
                    if (p.x > blob.maxX) blob.maxX = p.x;
                    if (p.y < blob.minY) blob.minY = p.y;
                    if (p.y > blob.maxY) blob.maxY = p.y;

                    // Check 8-connected neighbors
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = p.x + dx;
                            int ny = p.y + dy;
                            if (nx >= 0 && ny >= 0 && nx < pixelWidth && ny < pixelHeight) {
                                if (getPixelMask(nx, ny, mask, pixelWidth)) {
                                    clearPixelMask(nx, ny, mask, pixelWidth);
                                    queue.push_back({nx, ny});
                                }
                            }
                        }
                    }
                }

                // Compute centroid
                blob.centreX = (float)blob.sumX / blob.pixelCount;
                blob.centreY = (float)blob.sumY / blob.pixelCount;

                blobs.push_back(blob);
            
            }
        }
    }
    tempID = 0;
}



