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
#include <memory.h>

#include <efi_memory.h>
#include <integers.h>
#include <large_integers.h>

int memcmp(void* aPtr, void* bPtr, u64 numBytes) {
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
void memcpy(void* src, void* dest, u64 numBytes) {
    if (src == dest)
        return;

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
}

void memset(void* start, u8 value, u64 numBytes) {
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

void volatile_read(const volatile void* ptr, volatile void* out, u64 length) {
    if (ptr == nullptr || out == nullptr || length == 0)
        return;
    // FIXME: Are memory barries necessary for the memcpy call?
    if (length == 1)
        *(u8*)out = *(volatile u8*)ptr;
    else if (length == 2)
        *(u16*)out = *(volatile u16*)ptr;
    else if (length == 4)
        *(u32*)out = *(volatile u32*)ptr;
    else if (length == 8)
        *(u64*)out = *(volatile u64*)ptr;
    else memcpy((void*)ptr, (void*)out, length);
}

void volatile_write(void* data, volatile void* ptr, u64 length) {
    if (data == nullptr || ptr == nullptr || length == 0)
        return;
    // FIXME: Are memory barriers necessary for the memcpy call?
    if (length == 1)
        *(volatile u8*)ptr = *(u8*)data;
    if (length == 2)
        *(volatile u16*)ptr = *(u16*)data;
    if (length == 4)
        *(volatile u32*)ptr = *(u32*)data;
    if (length == 8)
        *(volatile u64*)ptr = *(u64*)data;
    else memcpy(data, (void*)ptr, length);
}
