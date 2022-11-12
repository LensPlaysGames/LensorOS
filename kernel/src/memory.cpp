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

#include <memory.h>

#include <integers.h>
#include <large_integers.h>

extern "C" int memcmp(const void* aPtr, const void* bPtr, size_t numBytes) {
    if (aPtr == bPtr)
        return 0;

    u8* a = (u8*)aPtr;
    u8* b = (u8*)bPtr;
    while (numBytes--) {
        if (*a != *b)
            return 1;
        a++;
        b++;
    }
    return 0;
}

// The signed comparison does limit `numBytes` to ~9 billion.
// I think I'm okay with that, as nobody will be moving 8192 pebibytes
//   around in memory any time soon. If you are, rewrite this, nerd :^)
extern "C" void* memcpy(void* __restrict__ dest, const void* __restrict__ src, size_t numBytes) {
    if (src == dest)
        return dest;

    s64 i = 0;
    for (; i <= (s64)numBytes - 2048; i += 2048)
        *(u16384*)((u64)dest + i) = *(u16384*)((u64)src + i);
    for (; i <= (s64)numBytes - 128; i += 128)
        *(u1024*)((u64)dest + i) = *(u1024*)((u64)src + i);
    for (; i <= (s64)numBytes - 32; i += 32)
        *(u256*)((u64)dest + i) = *(u256*)((u64)src + i);
    for (; i <= (s64)numBytes - 8; i += 8)
        *(u64*)((u64)dest + i) = *(u64*)((u64)src + i);
    for (; i < (s64)numBytes; ++i)
        *(u8*)((u64)dest + i) = *(u8*)((u64)src + i);

    return dest;
}

extern "C" void memset(void* start, u8 value, u64 numBytes) {
    if (numBytes >= 256) {
        u64 qWordValue = 0;
        qWordValue |= (u64)value << 0;
        qWordValue |= (u64)value << 8;
        qWordValue |= (u64)value << 16;
        qWordValue |= (u64)value << 24;
        qWordValue |= (u64)value << 32;
        qWordValue |= (u64)value << 40;
        qWordValue |= (u64)value << 48;
        qWordValue |= (u64)value << 56;
        u64 i = 0;
        for (; i <= numBytes - 8; i += 8)
            *(u64*)((u64)start + i) = qWordValue;
    }
    for (u64 i = 0; i < numBytes; ++i)
        *(u8*)((u64)start + i) = value;
}

