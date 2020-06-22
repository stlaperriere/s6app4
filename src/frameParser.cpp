#include "frameParser.h"

void FrameParser::acquireData() {
    if (state == PREAMBLE) {
        if (!FrameParser::validateInput(PREAMBLE_MASK)) isDataCorrupted = true;
        state = START;
    } else if (state == START) {
        if (!FrameParser::validateInput(START_MASK)) isDataCorrupted = true;
        state = TYPE_AND_FLAGS;
    } else if (state == TYPE_AND_FLAGS) {
        FrameParser::setTypeAndFlags();
        state = PAYLOAD_LENGTH;
    } else if (state == PAYLOAD_LENGTH) {
        if (!FrameParser::setPayloadLength()) isDataCorrupted = true;
        state = PAYLOAD;
    } else if (state == PAYLOAD) {
        if (payloadPointer < payloadLength) {
            FrameParser::appendPayload();
            state = PAYLOAD;
        } else {
            state = CONTROL;
        }
    } else if (state == CONTROL) {
        if (crcCounter < (CRC_LENGTH / 8)) {
            FrameParser::appendControl();
        }
        
        if (!FrameParser::validateControl()) isDataCorrupted = true;
        state = END;
    } else if (state == END) {
        if (!FrameParser::validateInput(END_MASK)) {
            isDataCorrupted = true;
        } else {
            isDataAvailable = true;
        }
    }
}

bool FrameParser::validateInput(uint8_t mask) {
    return *inputBuf == mask;
}

void FrameParser::setTypeAndFlags() {
    typeAndFlags = *inputBuf;
}

bool FrameParser::setPayloadLength() {
    payloadLength = *inputBuf;
    return *inputBuf < 80;
}

void FrameParser::appendControl() {
    crc = crc << 8;
    crc = crc | *inputBuf; // Ca va tu marcher?
    crcCounter++;
}

void FrameParser::appendPayload() {
    payload[payloadPointer++] = *inputBuf;
}

bool FrameParser::validateControl() {
    unsigned short calculatedCRC = crc16((char*) payload, payloadLength);
    return crc == (uint16_t) calculatedCRC;
}

uint8_t* FrameParser::getData() {
    return payload;
}

bool FrameParser::dataAvailable() {
    return isDataAvailable;
}