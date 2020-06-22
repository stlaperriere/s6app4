#include <list>
#include <vector>
#include <thread>

class FrameParser {
    public:
        FrameParser();
        void push(bool value);
        bool dataAvailable();
    
    protected:
        std::vector<bool> inputVector;
        std::list<bool> outputQueue;

        enum lengths {
            PREAMBLE = 8,
            START = 16,
            HEADER = 16,
            CONTROL = 16,
            END = 8,
        };
        
        enum endFlags {
            PREAMBLE_END = PREAMBLE,
            START_END = PREAMBLE + START,
            HEADER_END = PREAMBLE + START + HEADER,
        };

        enum masks {
            PREAMBLE_MASK = 0xAA,
            START_MASK = 0xFC,
            END_MASK = 0xFC
        }

        //std::vector<bool> preamble_mask = 

        int payloadLength = 0;
        int payloadEndFlag = 0;
        int controlEndFlag = 0;
        int endEndFlag = 0;

        bool isDataAvailable = false;
        bool isDataCorrupted = false;

        uint8_t preamble;
        uint8_t start;
        uint16_t header;
        

        void setPayloadLength(int length);
        void validatePreamble(std::vector<bool> preamble);
        void validateStart(std::vector<bool> start);

        bool equal(std::vector<bool> v1, std::vector<bool> v2);
}