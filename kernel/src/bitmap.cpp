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

#include <bitmap.h>
#include <integers.h>
#include <memory.h>

#include <algorithm>

Bitmap::Bitmap(u64 size, u8* bufferAddress)
    : Size(size)
    , Buffer(bufferAddress) {
    // Initialize the buffer to all zeros (ensure known state).
    if (Size and Buffer)
        memset(Buffer, 0, Size);
}

void Bitmap::move(u64 newSize, u8* bufferAddress) {
    // Initialize new buffer
    if (newSize and bufferAddress)
        memset(bufferAddress, 0, newSize);

    // Copy set bits from existing map to new map
    if (Size and Buffer and newSize and bufferAddress)
        memcpy(bufferAddress, Buffer, std::min(Size, newSize));

    // Update map
    Size = newSize;
    Buffer = bufferAddress;
}

bool Bitmap::get(u64 index) {
    u64 byteIndex = index / 8;
    if (byteIndex >= Size)
        return false;

    u8 bitIndexer = 0b10000000 >> (index % 8);
    return (Buffer[byteIndex] & bitIndexer) > 0;
}

bool Bitmap::set(u64 index, bool value) {
    u64 byteIndex = index / 8;
    if (byteIndex >= Size)
        return false;

    u8 bitIndexer = 0b10000000 >> (index % 8);
    Buffer[byteIndex] &= ~bitIndexer;
    if (value)
        Buffer[byteIndex] |= bitIndexer;

    return true;
}

bool Bitmap::operator[](u64 index) {
    return get(index);
}
