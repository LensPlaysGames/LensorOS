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

#ifndef LENSOR_OS_RANDOM_LINEAR_CONGRUENTIAL_GENERATOR_H
#define LENSOR_OS_RANDOM_LINEAR_CONGRUENTIAL_GENERATOR_H

#include <integers.h>

class LCG {
    u64 state{0};

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
