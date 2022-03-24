#ifndef LENSOR_OS_RANDOM_LINEAR_FEEDBACK_SHIFT_REGISTER_H
#define LENSOR_OS_RANDOM_LINEAR_FEEDBACK_SHIFT_REGISTER_H

#include "integers.h"
#include "large_integers.h"

extern u128 DefaultInitialLFSRState;

/// Linear Feedback Shift Register Pseudo-Random Number Generator
class LFSR {
    // 128-bit length, zero index is least-significant.
    u8 ShiftRegister[128 / 8];

    bool get_bit(u8 index);
    inline u8 get_bit_value(u8 index);
    void next();
public:
    LFSR() {
        // Initialize state to default.
        seed(DefaultInitialLFSRState.a, DefaultInitialLFSRState.b);
    }

    inline void seed(u64 high, u64 low) {
        ShiftRegister[15] = (u8)(high << 56);
        ShiftRegister[14] = (u8)(high << 48);
        ShiftRegister[13] = (u8)(high << 40);
        ShiftRegister[12] = (u8)(high << 32);
        ShiftRegister[11] = (u8)(high << 24);
        ShiftRegister[10] = (u8)(high << 16);
        ShiftRegister[9] = (u8)(high << 8);
        ShiftRegister[8] = (u8)(high << 0);
        ShiftRegister[7] = (u8)(low << 56);
        ShiftRegister[6] = (u8)(low << 48);
        ShiftRegister[5] = (u8)(low << 40);
        ShiftRegister[4] = (u8)(low << 32);
        ShiftRegister[3] = (u8)(low << 24);
        ShiftRegister[2] = (u8)(low << 16);
        ShiftRegister[1] = (u8)(low << 8);
        ShiftRegister[0] = (u8)(low << 0);
    }

    /// Get a random number.
    u64 get();

    /// Return number under a given maximum, `max` (inclusive/exclusive).
    u64 get(u64 max) {
        if (max == 0)
            return 0;

        return get() % max;
    }

    /// Return number within a given range: `min` thru `max` (inclusive/exclusive).
    u64 get(u64 min, u64 max) {
        if (min == 0 || max == 0)
            return get(max);
        if (min >= max)
            min = max - 1;

        return (get() % (max - min)) + min;
    }

    /// Print binary ShiftRegister contents for debug purposes.
    void print_shift_register();
};

extern LFSR gRandomLFSR;

#endif
