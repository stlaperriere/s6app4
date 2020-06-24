#include "frame.h"
#include "crc.h"
#include "manchester.h"

#define FRAME_MAX_LENGTH 90
#define FRAME_OVERHEAD_LENGTH 7
#define PAYLOAD_START_INDEX 4

SYSTEM_THREAD(ENABLED);

class FrameWriter {
    public:
        FrameWriter();
        void setFrame(uint8_t typeAndFlags, uint8_t* payload, uint8_t payloadLength);
        bool nextByte(uint8_t* writeBuf);
        uint8_t getBytePointer();
        void reset();
        bool frameReady();
    
    protected:
        uint8_t frame[FRAME_MAX_LENGTH];
        uint8_t frameLength;
        uint8_t bytePointer;
        bool isFrameReady;
};