#include "frameParser.h"

void FrameParser::acquireData(uint8_t* inputBuf) {
    if (state == PREAMBLE) {
        if (!FrameParser::validateInput(inputBuf, Frame::PREAMBLE_MASK)) isDataCorrupted = true;
        state = START;
    } else if (state == START) {
        if (!FrameParser::validateInput(inputBuf, Frame::START_MASK)) isDataCorrupted = true;
        state = TYPE_AND_FLAGS;
    } else if (state == TYPE_AND_FLAGS) {
        FrameParser::setTypeAndFlags(inputBuf);
        state = PAYLOAD_LENGTH;
    } else if (state == PAYLOAD_LENGTH) {
        if (!FrameParser::setPayloadLength(inputBuf)) isDataCorrupted = true;
        state = PAYLOAD;
    } else if (state == PAYLOAD) {
        if (payloadPointer < payloadLength) {
            FrameParser::appendPayload(inputBuf);
            state = PAYLOAD;
        } else {
            state = CONTROL;
        }
    } else if (state == CONTROL) {
        if (crcCounter < (CRC_LENGTH / 8)) {
            FrameParser::appendControl(inputBuf);
        }
        
        if (!FrameParser::validateControl()) isDataCorrupted = true;
        state = END;
    } else if (state == END) {
        if (!FrameParser::validateInput(inputBuf, Frame::END_MASK)) {
            isDataCorrupted = true;
        } else {
            isDataAvailable = true;
            
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Frame COMPLETE!");
            }
        }
    }
}

bool FrameParser::validateInput(uint8_t* inputBuf, uint8_t mask) {
    return *inputBuf == mask;
}

void FrameParser::setTypeAndFlags(uint8_t* inputBuf) {
    typeAndFlags = *inputBuf;
}

bool FrameParser::setPayloadLength(uint8_t* inputBuf) {
    payloadLength = *inputBuf;
    return *inputBuf < 80;
}

void FrameParser::appendControl(uint8_t* inputBuf) {
    crc = crc << 8;
    crc = crc | *inputBuf; // Ca va tu marcher?
    crcCounter++;
}

void FrameParser::appendPayload(uint8_t* inputBuf) {
    payload[payloadPointer++] = *inputBuf;
}

bool FrameParser::validateControl() {
    unsigned short calculatedCRC = CRC::crc16((char*) payload, payloadLength);
    return crc == (uint16_t) calculatedCRC;
}

uint8_t FrameParser::getData(uint8_t* data) {
    data = payload;
    return payloadLength;
}

bool FrameParser::dataAvailable() {
    return isDataAvailable;
}