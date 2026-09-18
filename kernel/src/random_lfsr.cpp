/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#include <cstr.h>
#include <random_lfsr.h>

#include <format>

LFSR gRandomLFSR;

u128 DefaultInitialLFSRState = {(u64)1 << 63, (u64)1};

void LFSR::print_shift_register() {
    std::print("[LFSR]: Shift Register Contents: 0b");
    for (u8 i = 128 / 8 - 1; i < 128 / 8; --i) {
        for (u8 j = 8 - 1; j < 8; --j) {
            u8 bitIndex = (i * 8 + j);
            std::print("{}", get_bit_value(bitIndex));
        }
    }
    std::print("\n");
}

void LFSR::next() {
    // left-most bit = new bit to shift into register.
    u8 newbit = ((get_bit_value(0) ^ (get_bit_value(1))
                  ^ (get_bit_value(2)) ^ (get_bit_value(7)))
                 & 1)
                << 7;

    // Shift each byte within shift register
    u8 lastCarry{0};
    for (u8 j = 128 / 8 - 1; j < 128 / 8; --j) {
        // left-most bit = bit that will be shifted out.
        u8 carry = (ShiftRegister[j] & 1) << 7;
        // Shift byte within shift register, taking into
        //   account previous byte's shifted value.
        ShiftRegister[j] = (ShiftRegister[j] >> 1) | lastCarry;
        // Save carry bit from this byte for next byte.
        lastCarry = carry;
    }

    ShiftRegister[128 / 8 - 1] |= newbit;
}

// `index` counts from right side.
bool LFSR::get_bit(u8 index) {
    return (ShiftRegister[index / 8] & (0b00000001 << (index % 8))) > 0;
}

u8 LFSR::get_bit_value(u8 index) {
    return get_bit(index) ? 1 : 0;
}

u64 LFSR::get() {
    u64 result{0};
    for (u8 i = 0; i < 64; ++i) {
        if (get_bit(0))
            result |= 1;

        result = result << 1;
        next();
    }
    return result;
}
