#ifndef LENSOR_OS_RANDOM_LINEAR_FEEDBACK_SHIFT_REGISTER_H
#define LENSOR_OS_RANDOM_LINEAR_FEEDBACK_SHIFT_REGISTER_H

#include "integers.h"

/// Linear Feedback Shift Register Pseudo-Random Number Generator
class LFSR {
    // 128-bit length, zero index is least-significant.
    u8 ShiftRegister[128 / 8];

    bool get_bit(u8 index);
    inline u8 get_bit_value(u8 index);
    void next();
public:
    LFSR() {
        // Initialize state.
        ShiftRegister[15] = 0b10000000;
        ShiftRegister[14] = 0b00000000;
        ShiftRegister[13] = 0b00000000;
        ShiftRegister[12] = 0b00000000;
        ShiftRegister[11] = 0b00000000;
        ShiftRegister[10] = 0b00000000;
        ShiftRegister[9]  = 0b00000000;
        ShiftRegister[8]  = 0b00000000;
        ShiftRegister[7]  = 0b00000000;
        ShiftRegister[6]  = 0b00000000;
        ShiftRegister[5]  = 0b00000000;
        ShiftRegister[4]  = 0b00000000;
        ShiftRegister[3]  = 0b00000000;
        ShiftRegister[2]  = 0b00000000;
        ShiftRegister[1]  = 0b00000000;
        ShiftRegister[0]  = 0b00000001;
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
