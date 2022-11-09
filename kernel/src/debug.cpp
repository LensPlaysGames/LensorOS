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

#include "panic.h"
#include "string"

#include <basic_renderer.h>
#include <cstr.h>
#include <debug.h>
#include <integers.h>
#include <string.h>
#include <uart.h>
#include <va_list.h>

void dbgmsg_c(char c) {
    UART::outc(c);
}

void dbgmsg_s(const char* str) {
    UART::out(str);
}

void dbgmsg_buf(const u8* buffer, u64 byteCount) {
    UART::out(buffer, byteCount);
}

void dbgmsg(char character, ShouldNewline nl) {
    dbgmsg_c(character);
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg(const String& str, ShouldNewline nl) {
    dbgmsg_buf(str.bytes(), str.length());
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg(u8* buffer, u64 byteCount, ShouldNewline nl) {
    dbgmsg_buf(buffer, byteCount);
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg(bool value, ShouldNewline nl) {
    dbgmsg_s(value ? trueString : falseString);
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg(double number, ShouldNewline nl) {
    UART::out(to_string(number));
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg(s64 number, ShouldNewline nl) {
    dbgmsg_s(to_string(number));
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg(s32 number, ShouldNewline nl) {
    dbgmsg_s(to_string(number));
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg(s16 number, ShouldNewline nl) {
    dbgmsg_s(to_string(number));
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg(s8 number, ShouldNewline nl) {
    dbgmsg_s(to_string(number));
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg(u64 number, ShouldNewline nl) {
    dbgmsg_s(to_string(number));
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}
void dbgmsg(u32 number, ShouldNewline nl) {
    dbgmsg_s(to_string(number));
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}
void dbgmsg(u16 number, ShouldNewline nl) {
    dbgmsg_s(to_string(number));
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}
void dbgmsg(u8 number, ShouldNewline nl) {
    dbgmsg_s(to_string(number));
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgmsg_v(const u8* fmt, va_list args) {
    const u8* current = fmt;
    for(; *current != '\0'; ++current) {
#ifdef LENSOR_OS_UART_HIDE_COLOR_CODES
        if (*current == '\033') {
            do {
                current++;
            } while(*current != 'm' && *current != '\0');
            if (*current == '\0')
                return;

            current++;
            if (*current == '\0')
                return;
        }
#endif /* LENSOR_OS_UART_HIDE_COLOR_CODES */
        if (*current == '%') {
            current++;
            switch (*current) {
            default:
                // If char following `%` is unrecognized, pass through unchanged.
                current--;
                dbgmsg(static_cast<char>(*current));
                break;
            case '\0':
                // Handle '%' at end of format string.
                dbgmsg(static_cast<char>('%'));
                return;
            case 's':
                // %s
                current++;
                if (*current != 'l') {
                    // Found %s -- string
                    dbgmsg(va_arg(args, const char*));
                    current--;
                    break;
                }
                dbgmsg(*(va_arg(args, const String*)));
                break;
            case 'h':
                // %h
                current++;
                // %h? <- ? is *current
                if (*current != 'i'
                    && *current != 'u'
                    && *current != 'h')
                {
                    // Found %h -- nothing
                    current--;
                    break;
                }
                // %hi -- 16 bit signed integer
                // %hu -- 16 bit unsigned integer
                // %hh -- Lookahead for %hhi or %hhu
                if (*current == 'i')
                    dbgmsg(static_cast<s16>(va_arg(args, int)));
                if (*current == 'u')
                    dbgmsg(static_cast<u16>(va_arg(args, int)));
                if (*current == 'h') {
                    current++;
                    if (*current != 'i'
                        && *current != 'u')
                    {
                        // Found %hh -- nothing
                        current--;
                        break;
                    }
                    if (*current == 'i') {
                        // Found %hhi -- 8 bit signed integer
                        dbgmsg(static_cast<s8>(va_arg(args, int)));
                        break;
                    }
                    if (*current == 'u') {
                        // Found %hhu -- 8 bit unsigned integer
                        dbgmsg(static_cast<u8>(va_arg(args, int)));
                        break;
                    }
                }
                break;
            case 'u':
                current++;
                if (*current != 'l') {
                    // Found %u -- native bit unsigned integer
                    dbgmsg(va_arg(args, unsigned));
                    current--;
                    break;
                }
                current++;
                if (*current != 'l') {
                    // Found %ul -- 32 bit unsigned integer
                    dbgmsg(va_arg(args, u32));
                    current--;
                    break;
                }
                // Found %ull -- 64 bit unsigned integer
                dbgmsg(va_arg(args, u64));
                break;
            case 'd':
            case 'i':
                current++;
                if (*current != 'l') {
                    // Found %i -- native bit signed integer
                    dbgmsg(va_arg(args, signed));
                    current--;
                    break;
                }
                current++;
                if (*current != 'l') {
                    // Found %il -- 32 bit unsigned integer
                    dbgmsg(va_arg(args, s32));
                    current--;
                    break;
                }
                // Found %ill -- 64 bit unsigned integer
                dbgmsg(va_arg(args, s64));
                break;
            case 'f':
                dbgmsg((va_arg(args, double)));
                break;
            case 'p':
            case 'x':
                dbgmsg_s("0x");
                dbgmsg_s(to_hexstring(va_arg(args, u64)));
                break;
            case 'c':
                dbgmsg(static_cast<char>(va_arg(args, int)));
                break;
            case 'b':
                dbgmsg(static_cast<bool>(va_arg(args, int)));
                break;
            }
        }
        else dbgmsg(static_cast<char>(*current));
    }
}

void dbgmsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    dbgmsg_v(reinterpret_cast<const u8*>(fmt), args);
    va_end(args);
}

void dbgrainbow(const String& str, ShouldNewline nl) {
    for (u64 i = 0; i < str.length(); ++i) {
        dbgmsg("\033[1;3%im", i % 6 + 1);
        dbgmsg_c(str[i]);
    }
    dbgmsg_s("\033[0m");
    if (nl == ShouldNewline::Yes)
        dbgmsg_s("\r\n");
}

void dbgrainbow(const char* str, ShouldNewline nl) {
    dbgrainbow(String(str), nl);
}
