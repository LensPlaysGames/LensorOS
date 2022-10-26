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

#ifndef LENSOR_OS_BITMAP_H
#define LENSOR_OS_BITMAP_H

#include <integers.h>

class Bitmap {
public:
    Bitmap() {}

    Bitmap(u64 size, u8* bufferAddress);

    void init(u64 size, u8* bufferAddress);
    u64 length() { return Size; }
    void* base() { return (void*)Buffer; };

    bool get(u64 index);
    bool set(u64 index, bool value);

    bool operator [] (u64 index);

private:
    /* Number of bits within the bitmap. */
    u64 Size;
    /* Buffer to store bitmap within. */
    u8* Buffer;
};

#endif
