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

const char to_hex_not_supported[] = "TYPE_NOT_SUPPORTED";

extern "C" size_t strlen(const char* a) {
    size_t out = 0;
    while (true) {
        if (*a == '\0')
            return out + 1;

        ++a;
        ++out;
    }
}

bool strcmp(const char* a, const char* b, u64 length) {
    for (u64 i = 0; i < length; ++i) {
        if (*a != *b)
            return false;

        ++a;
        ++b;
    }
    return true;
}
