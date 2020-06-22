#include "frameWriter.h"

int FrameWriter::writeFrame(uint8_t* writeBuf, uint8_t typeAndFlags, uint8_t* payload, uint8_t payloadLength){
    if (payloadLength <= 80 && payloadLength >= 0) {
        try {
            writeBuf[0] = Frame::PREAMBLE_MASK;
            writeBuf[1] = Frame::START_MASK;
            writeBuf[2] = typeAndFlags;
            writeBuf[4] = payloadLength;

            for (int i = 5; i < payloadLength; i++) {
                writeBuf[i] = payload[i-5];
            }

            unsigned short crc = CRC::crc16((char*) payload, payloadLength);
            writeBuf[payloadLength] = crc >> 8;
            writeBuf[payloadLength + 1] = crc;

            writeBuf[payloadLength + 2] = Frame::END_MASK;
            
            return 0;
        } catch (int e) {
            return -1;
        }
    } else {
        return -1;
    }
}