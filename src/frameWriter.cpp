#include "frameWriter.h"

FrameWriter::FrameWriter() {
    this->reset();
}

void FrameWriter::setFrame(uint8_t typeAndFlags, uint8_t* payload, uint8_t payloadLength){
    isFrameReady = false;

    if (FrameWriter::generateFrame(typeAndFlags, payload, payloadLength)) isFrameReady = true;
}

void FrameWriter::setFaultyFrame(uint8_t typeAndFlags, uint8_t* payload, uint8_t payloadLength, int faultyBitIndex) {
    isFrameReady = false;

    if (FrameWriter::generateFrame(typeAndFlags, payload, payloadLength)) {
        FrameWriter::setFaultyBit(faultyBitIndex);
        isFrameReady = true;
    }
}

bool FrameWriter::nextByte(uint8_t* writeBuf) {
    if (bytePointer < frameLength) {
        *writeBuf = frame[bytePointer++];
        return true;
    } else {
        this->reset();
        isFrameReady = false;
        return false;
    }
    
}

void FrameWriter::reset() {
    for (int i = 0; i < FRAME_MAX_LENGTH; i++) {
        frame[i] = 0;
    }

    bytePointer = 0;
    isFrameReady = false;
}

bool FrameWriter::frameReady() {
    return isFrameReady;
}

uint8_t FrameWriter::getBytePointer() {
    return bytePointer;
}

void FrameWriter::setFaultyBit(int index) {
    if (index < frameLength * 8) {
        uint8_t offset = index % 8;
        uint8_t corruptionMask = 1 << offset;
        frame[index/8] = frame[index/8] xor corruptionMask; // Invert bit
    }
}

bool FrameWriter::generateFrame(uint8_t typeAndFlags, uint8_t* payload, uint8_t payloadLength) {
        if (payloadLength <= 80 && payloadLength >= 0) {
        frameLength = FRAME_OVERHEAD_LENGTH + payloadLength;

        frame[0] = Frame::PREAMBLE_MASK;
        frame[1] = Frame::START_MASK;
        frame[2] = typeAndFlags;
        frame[3] = payloadLength;

        for (int i = PAYLOAD_START_INDEX; i < PAYLOAD_START_INDEX + payloadLength; i++) {
            frame[i] = payload[i - PAYLOAD_START_INDEX];
        }

        unsigned short crc = CRC::crc16((char*) payload, payloadLength);
        frame[PAYLOAD_START_INDEX + payloadLength] = crc >> 8;
        frame[PAYLOAD_START_INDEX + payloadLength + 1] = crc;

        frame[PAYLOAD_START_INDEX + payloadLength + 2] = Frame::END_MASK;

        return true;
    } else {
        return false;
    }
}