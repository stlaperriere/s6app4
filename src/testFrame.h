namespace TestFrame {
    uint8_t testData[] = {
        0x55, // Preamble
        0xFC, // Start
        0x11, // Type and flags
        0x04, // payload Length (in bytes)
        0x11, // Payload[0]
        0x22, // Payload[1]
        0x33, // Payload[2]
        0x44, // Payload[3]
        0xAD, // CRC[0]
        0x0D, // CRC16[1]
        0xFC // End
    };

    uint8_t testPayload[] = {
        0x11,
        0x22,
        0x33,
        0x44
    };

    uint8_t testPayloadLength = 4;
};