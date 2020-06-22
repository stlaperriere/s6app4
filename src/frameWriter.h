#include "frame.h"
#include "crc.h"

class FrameWriter {
    public:
        int writeFrame(uint8_t* writeBuf, uint8_t typeAndFlags, uint8_t* payload, uint8_t payloadLength);
};
