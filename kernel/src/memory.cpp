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

#include <integers.h>
#include <large_integers.h>
#include <memory.h>

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

extern "C" void* memcpy(void* __restrict__ dest, const void* __restrict__ src, size_t numBytes) {
    void* result = dest;

    asm volatile(
        "shrq $3, %%rcx\n\t"  // Divide byte count by 8 to get QWORD count
        "rep movsq\n\t"       // Copy 8-byte blocks. Modifies RDI and RSI.

        "movq %[numBytes], %%rcx\n\t"  // Reload original byte count
        "andq $7, %%rcx\n\t"           // Extract trailing bytes (numBytes % 8)
        "rep movsb\n\t"                // Copy remaining bytes. Modifies RDI and RSI.

        : "+D"(dest), "+S"(src)                    // Tells compiler RDI and RSI are modified in-place
        : "c"(numBytes), [numBytes] "r"(numBytes)  // Pass size into RCX ("c") and another GPR
        : "memory"                                 // Memory barrier to protect cache sequencing
    );

    return result;
}

extern "C" void memset(void* start, u8 value, u64 numBytes) {
    u64 i = 0;
    if (numBytes >= 256) {
        u64 qWordValue = value * 0x0101010101010101ull;
        // qWordValue |= (u64)value << 0;
        // qWordValue |= (u64)value << 8;
        // qWordValue |= (u64)value << 16;
        // qWordValue |= (u64)value << 24;
        // qWordValue |= (u64)value << 32;
        // qWordValue |= (u64)value << 40;
        // qWordValue |= (u64)value << 48;
        // qWordValue |= (u64)value << 56;
        for (; i + 8 <= numBytes; i += 8)
            *(u64*)((u8*)start + i) = qWordValue;
    }
    for (; i < numBytes; ++i)
        *(u8*)((u8*)start + i) = value;
}

extern "C" void* memmove(void* dst, const void* src, size_t num) {
    if (num == 0 || src == dst) return dst;
    if (src > dst)
        return memcpy(dst, src, num);
    else
        for (usz i = num; i; --i)
            ((u8*)dst)[i - 1] = ((u8*)src)[i - 1];
    return dst;
}

extern "C" void* memchr(const void* __mem, int __char, size_t __n) {
    auto* block = (const unsigned char*)__mem;
    for (size_t i = 0; i < __n; ++i)
        if (block[i] == (unsigned char)__char)
            return (void*)((size_t)__mem + i);
    return nullptr;
}

extern "C" const void* memmem(const void* __haystack, size_t __haystacklen, const void* __needle, size_t __needlelen) {
    if (__needlelen > __haystacklen) return nullptr;
    // Stop searching at the last possible position for a match,
    // which is_haystack[ haystacklen - needlelen + 1 ].
    __haystacklen -= __needlelen - 1;
    while (__haystacklen) {
        // Find the first byte in a potential match
        unsigned char* z = (unsigned char*)memchr(
            (unsigned char*)__haystack,
            *(unsigned char*)__needle,
            __haystacklen);
        if (!z) return nullptr;
        // Check if there is enough space for there to actually be a match.
        ptrdiff_t delta = z - (unsigned char*)__haystack;
        ptrdiff_t remaining = (ptrdiff_t)__haystacklen - delta;
        if (remaining < 1) return nullptr;
        // Advance pointer and update the amount of __haystack remaining.
        __haystacklen -= delta;
        __haystack = z;
        // Did we find a match?
        if (!memcmp(__haystack, __needle, __needlelen)) return __haystack;
        // Ready for next loop
        __haystack = (unsigned char*)__haystack + 1;
        __haystacklen -= 1;
    }
    return nullptr;
}
