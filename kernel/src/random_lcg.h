#ifndef LENSOR_OS_RANDOM_LINEAR_CONGRUENTIAL_GENERATOR_H
#define LENSOR_OS_RANDOM_LINEAR_CONGRUENTIAL_GENERATOR_H

#include <integers.h>

class LCG {
    u64 state {0};
public:
    inline void seed(u64 s) {
        state = s;
    }

    inline void next() {
        state = 1103515245 * state + 12345;
    }

    /// Get a random 64-bit number.
    u64 get();
    /// Get a random number between zero and given `max` (inclusive/exclusive).
    u64 get(u64 max);
    /// Get a random number between a given `min` and `max` (inclusive/exclusive).
    u64 get(u64 min, u64 max);
};

extern LCG gRandomLCG;

#endif
