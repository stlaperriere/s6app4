#include <Particle.h>

namespace Frame {
    enum Mask { // Expected values for certain fields
        PREAMBLE_MASK = 0xAA,
        START_MASK = 0xFC,
        END_MASK = 0xFC
    };
}