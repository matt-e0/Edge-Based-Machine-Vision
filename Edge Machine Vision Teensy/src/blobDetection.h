class Blob {
    private:
        uint8_t xIndex;
        uint8_t yIndex;
        uint_fast8_t size;
        Blob(int x, int y, int s);

    public:
        int getX() {
            return xIndex;
        }
        void setX(int x) {
            xIndex = x;
        }
        int getY() {
            return yIndex;
        }
        void setY(int y) {
            xIndex = y;
        }

};

Blob::Blob(int x, int y, int s) {
    xIndex = x;
    yIndex = y;
    size = s;
}

Blob blobs[];

void detectBlobs(uint8_t mask[]) {

}

int getBlobPositions() {

}