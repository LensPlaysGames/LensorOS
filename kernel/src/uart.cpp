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
#include <panic.h>
#include <uart.h>

namespace UART {
void out(u8 byte) {
    if (byte == '\0') {
        out_raw('\\');
        out_raw('0');
        // out_raw('N');
        // out_raw('U');
        // out_raw('L');
    }
    if (byte == '\n') out_raw('\r');
    out_raw(byte);
}

void out(std::string_view str) {
    if (!initialized())
        return;

    const char* c = str.data();
    size_t remaining = str.size();

    while (remaining > 0) {
#ifdef LENSOR_OS_UART_HIDE_COLOR_CODES
        if (*c == '\033') {
            // Loop safely within the boundaries of the string view size
            do {
                c++;
                remaining--;
            } while (remaining > 0 && *c != 'm');

            if (remaining > 0) {
                c++;  // Skip the 'm'
                remaining--;
            }
            continue;
        }
#endif /* defined LENSOR_OS_UART_HIDE_COLOR_CODES */

        out(u8(*c));
        c++;
        remaining--;
    }
}

void out(const u8* str, u64 numberOfBytes) {
    if (initialized() == false)
        return;

    /// Sanity check.
    if (numberOfBytes > 1'000'000'000'000llu) {
        std::print("[UART]: Refusing to output more than 1'000'000'000'000 bytes\n");
        panic("Buffer overflow");
        hang();
    }

    while (numberOfBytes > 0) {
#ifdef LENSOR_OS_UART_HIDE_COLOR_CODES
        if (*str == '\033') {
            // Loop until no more chars or 'm'
            do {
                str++;
                numberOfBytes--;
            } while (*str != 'm' && numberOfBytes > 0);

            if (numberOfBytes > 0) {
                str++;
                numberOfBytes--;
            }

            continue;
        }
#endif /* defined LENSOR_OS_UART_HIDE_COLOR_CODES */
        out(*str);
        str++;
        numberOfBytes--;
    }
}

}  // namespace UART
