#include "frame.h"

FrameParser::FrameParser() {

}

void FrameParser::push(bool value) {
    FrameParser::inputVector.push_back(value);

    if (FrameParser::inputVector.size() == FrameParser::PREAMBLE_END) {
        FrameParser::validatePreamble(std::vector<bool>(0, FrameParser::PREAMBLE_END));
    } else if (FrameParser::inputVector.size() == FrameParser::START_END) {
        //valider start
    } else if (FrameParser::inputVector.size() == FrameParser::HEADER_END) {
        // Extraire flags
        // Extraire longueur du payload
    }else if (FrameParser::inputVector.size() == FrameParser::payloadEndFlag) {
        // Extraire payload
    } else if (FrameParser::inputVector.size() == FrameParser::controlEndFlag) {
        // valider CRC
    } else if (FrameParser::inputVector.size() == FrameParser::endEndFlag) {
        // Valider end
        // si c'est ok, 
        // FrameParser::isDataAvailable = true;
        // transferer le payload dans outputQueue
    }
}

void FrameParser::setPayloadLength(int length) {
    FrameParser::payloadLength = length;
    FrameParser::payloadEndFlag = HEADER_END + length;
    FrameParser::controlEndFlag = FrameParser::payloadEndFlag + FrameParser::CONTROL;
    FrameParser::endEndFlag = FrameParser::controlEndFlag + FrameParser::END;
}

bool FrameParser::dataAvailable() {
    return FrameParser::isDataAvailable;
}

void FrameParser::validatePreamble(std::vector<bool> preamble) {
    if (preamble != FrameParser::PREAMBLE_MASK) {
        FrameParser::isDataCorrupted = true;
    } else {
        
    }
}

void validateStart(std::vector<bool> start) {

}

bool equal(std::vector<bool> v1, std::vector<bool> v2) {
    if (v1.size() == v2.size()) {
        for (int i = 0; i < v1.size(); i++) {
            if (v1[i] != v2[i]) return false;
        }

        return true;
    } else {
        return false;
    }
}