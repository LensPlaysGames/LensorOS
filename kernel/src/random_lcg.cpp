/* Copyright 2022, Contributors To LensorOS.
All rights reserved.

This file is part of LensorOS.

LensorOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LensorOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LensorOS. If not, see <https://www.gnu.org/licenses */
#include <random_lcg.h>

#include <integers.h>

LCG gRandomLCG;

u64 LCG::get() {
    next();
    return state;
}

u64 LCG::get(u64 max) {
    if (max == 0)
        return 0;
    next();
    return state % max;
}

u64 LCG::get(u64 min, u64 max) {
    if (min == 0 || max == 0)
        return get(max);
    if (min >= max)
        min = max - 1;
    next();
    return (state % (max - min)) + min;
}
