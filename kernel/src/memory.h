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


#ifndef LENSOR_OS_MEMORY_H
#define LENSOR_OS_MEMORY_H

#include <integers.h>

extern "C" void memset(void* src, u8 value  , u64 numBytes);
extern "C" int memcmp(const void* src, const void* dest, size_t numBytes);
extern "C" void* memcpy(void* __restrict__ dest, const void* __restrict__ src, size_t numBytes);

template <typename T>
T volatile_read(const volatile T* ptr) {
    return *ptr;
}

template <typename T>
void volatile_write(volatile T* dest, const T& value) {
    *dest = value;
}

#endif
