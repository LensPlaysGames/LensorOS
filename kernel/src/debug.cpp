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

#include <format>

#include <debug.h>
#include <integers.h>
#include <string.h>
#include <uart.h>

void dbgmsg_buf(const u8* buffer, u64 byteCount) {
    UART::out(buffer, byteCount);
}

void dbgrainbow(const String& str, ShouldNewline nl) {
    for (u64 i = 0; i < str.length(); ++i) {
        std::print("\033[1;3{}m{}", i % 6 + 1, char(str[i]));
    }
    std::print("\033[0m");
    if (nl == ShouldNewline::Yes)
        std::print("\n");
}

void dbgrainbow(const char* str, ShouldNewline nl) {
    dbgrainbow(String(str), nl);
}
