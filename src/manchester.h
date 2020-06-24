#include "Particle.h"

namespace Manchester
{
    void init(void (*onByteReceivedCallback)(const uint8_t&));
    void send(const uint8_t& data);
}