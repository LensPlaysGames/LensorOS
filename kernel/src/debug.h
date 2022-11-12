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

#ifndef LENSOR_OS_DEBUG_H
#define LENSOR_OS_DEBUG_H

#include <integers.h>
#include <string>
#include <va_list.h>

enum class ShouldNewline {
    Yes = 0,
    No = 1
};

/// Print a number of bytes from a given buffer as characters.
void dbgmsg_buf(const u8* buffer, u64 byteCount);

/// Print a string with lots of colors (and no formatting)! Nyan debug :^)
void dbgrainbow(std::string_view, ShouldNewline nl = ShouldNewline::No);

/// Print a C-style null-terminated string in lots
/// of colors (and no formatting)! Nyan debug :^)
void dbgrainbow(const char* str, ShouldNewline nl = ShouldNewline::No);

#endif /* LENSOR_OS_DEBUG_H */
