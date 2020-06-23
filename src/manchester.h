#include "Particle.h"

namespace Manchester
{
    void init(void (*onBufferFilledCallback)(uint8_t*));
    void send(uint8_t* data);
}