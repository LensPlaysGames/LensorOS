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

// Yes, I know these are "backwards" parameters, it's just how
// I started and I don't want to go back and change everything.
int memcmp(void* src, void* dest, u64 numBytes);
void memcpy(void* src, void* dest, u64 numBytes);
void memset(void* src, u8 value  , u64 numBytes);

void volatile_read(const volatile void* ptr, volatile void* out, u64 length);
void volatile_write(void* data, volatile void* ptr, u64 length);

#endif
