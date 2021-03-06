#ifndef FRAME_H
#define FRAME_H

#include <Particle.h>

namespace Frame {
    enum Mask { // Expected values for certain fields
        PREAMBLE_MASK = 0x55,
        START_MASK = 0xFC,
        END_MASK = 0xFC
    };
}

#endif