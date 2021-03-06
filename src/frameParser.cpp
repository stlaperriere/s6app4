#include "frameParser.h"

void FrameParser::acquireData(const uint8_t& inputBuf) {
    if (state == PREAMBLE) {
        if (!FrameParser::validateInput(inputBuf, Frame::PREAMBLE_MASK)) FrameParser::raiseError();
        state = START;
    } else if (state == START) {
        if (!FrameParser::validateInput(inputBuf, Frame::START_MASK)) FrameParser::raiseError();
        state = TYPE_AND_FLAGS;
    } else if (state == TYPE_AND_FLAGS) {
        FrameParser::setTypeAndFlags(inputBuf);
        state = PAYLOAD_LENGTH;
    } else if (state == PAYLOAD_LENGTH) {
        if (!FrameParser::setPayloadLength(inputBuf)) FrameParser::raiseError();
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
        
        if (!FrameParser::validateControl()) FrameParser::raiseError();
        state = END;
    } else if (state == END) {
        if (!FrameParser::validateInput(inputBuf, Frame::END_MASK)) {
            FrameParser::raiseError();
        } 
        
        if (!isDataCorrupted) {
            isDataAvailable = true;
            
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Frame SUCCES!");
            }
        } else {
            WITH_LOCK(Serial)
            {
                Serial.printlnf("Frame FAIL!");
            }
            FrameParser::reset();
        }
    } else {
        FrameParser::raiseError();
    }
}

bool FrameParser::validateInput(const uint8_t& inputBuf, uint8_t mask) {
    return inputBuf == mask;
}

void FrameParser::setTypeAndFlags(const uint8_t& inputBuf) {
    typeAndFlags = inputBuf;
}

bool FrameParser::setPayloadLength(const uint8_t& inputBuf) {
    payloadLength = inputBuf;
    return inputBuf < 80;
}

void FrameParser::appendControl(const uint8_t& inputBuf) {
    crc = crc << 8;
    crc = crc | inputBuf; // Ca va tu marcher?
    crcCounter++;
}

void FrameParser::appendPayload(const uint8_t& inputBuf) {
    payload[payloadPointer++] = inputBuf;
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

void FrameParser::reset() {
    isDataAvailable = false;
    isDataCorrupted = false;
    payloadLength = 0;
    payloadPointer = 0;
    crc = 0;
    crcCounter = 0;

    for (int i = 0; i < 80; i++) {
        payload[i] = 0;
    }

    state = PREAMBLE;

    WITH_LOCK(Serial) {
        Serial.printlnf("FrameParser reset");
    }
}

void FrameParser::raiseError() {
    isDataCorrupted = true;
    isDataAvailable = false;

    WITH_LOCK(Serial) {
        Serial.printlnf("Frame HAS ERRORS!");
    }
}