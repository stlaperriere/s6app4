#include "crc.h"
#include "frame.h"
#define CRC_LENGTH 16

class FrameParser {
    public:
        uint8_t getData(uint8_t* data);
        bool dataAvailable();
        void acquireData(const uint8_t& inputBuf);

    protected:
        enum State { // Each field has its state
            PREAMBLE,
            START,
            TYPE_AND_FLAGS,
            PAYLOAD_LENGTH,
            PAYLOAD,
            CONTROL,
            END,
        };

        // const uint8_t& inputBuf; // Pointer to the Manchester output buffer

        State state; // Current state for the FSM

        bool isDataCorrupted = false; // Indicated whether there are errors in the frame
        bool isDataAvailable = false; // Indicates whether the frame is complete and without errors

        uint8_t typeAndFlags;
        uint8_t payloadLength = 0;
        uint8_t payloadPointer = 0;
        uint8_t payload[80];
        uint16_t crc = 0;
        uint8_t crcCounter = 0;
        
        bool validateInput(const uint8_t& inputBuf, uint8_t mask);
        void setTypeAndFlags(const uint8_t& inputBuf);
        bool setPayloadLength(const uint8_t& inputBuf);
        void appendPayload(const uint8_t& inputBuf);
        void appendControl(const uint8_t& inputBuf);
        bool validateControl();
};
