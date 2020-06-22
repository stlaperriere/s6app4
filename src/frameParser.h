#include "crc.h"
#define CRC_LENGTH 16

class FrameParser {
    public:
        FrameParser(); // Probablement specifier le buf de sortie du parser manchester
        uint8_t* getData();
        bool dataAvailable();

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

        enum Mask { // Expected values for certain fields
            PREAMBLE_MASK = 0xAA,
            START_MASK = 0xFC,
            END_MASK = 0xFC
        };

        uint8_t* inputBuf; // Pointer to the Manchester output buffer

        State state; // Current state for the FSM

        bool isDataCorrupted = false; // Indicated whether there are errors in the frame
        bool isDataAvailable = false; // Indicates whether the frame is complete and without errors

        uint8_t typeAndFlags;
        uint8_t payloadLength = 0;
        uint8_t payloadPointer = 0;
        uint8_t payload[80];
        uint16_t crc = 0;
        uint8_t crcCounter = 0;
        
        void acquireData();
        bool validateInput(uint8_t mask);
        void setTypeAndFlags();
        bool setPayloadLength();
        void appendPayload();
        void appendControl();
        bool validateControl();
}
