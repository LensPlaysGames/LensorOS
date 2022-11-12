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

#ifndef LENSOR_OS_CSTR_H
#define LENSOR_OS_CSTR_H

#include <integers.h>

#ifndef LENSOR_OS_UART_HIDE_COLOR_CODES
constexpr const char* trueString = "\033[32mTrue\033[0m";
constexpr const char* falseString = "\033[31mFalse\033[0m";
#else
constexpr const char* trueString = "True";
constexpr const char* falseString = "False";
#endif /* #ifndef LENSOR_OS_UART_HIDE_COLOR_CODES */

/* String Length
 *   Returns the number of characters including null terminator.
 */
extern "C" size_t strlen(const char* a);

/* String Compare
 *   Returns `true` only if all characters within
 *   both strings up to length are exactly equal.
 */
bool strcmp(const char* a, const char* b, u64 length);

const char* to_string(bool);

constexpr u64 LENSOR_OS_TO_STRING_BUF_SZ = 20;
constexpr u64 LENSOR_OS_TO_STRING_BUF_SZ_DBL = 40;

char* to_string(u64);
char* to_string(u32);
char* to_string(u16);
char* to_string(u8);
char* to_string(s64);
char* to_string(s32);
char* to_string(s16);
char* to_string(s8);
char* to_string(double, u8 decimalPlaces = 2);
char* to_hexstring(u64 value, bool capital = false);
extern const char to_hex_not_supported[];
template <typename T>
char* to_hexstring(T value, bool capital = false) {
    // FIXME: This is really, really bad!
    u8 sz = sizeof(T);
    if (sz == 1)
        return to_hexstring((u64)value, capital);
    if (sz == 2)
        return to_hexstring((u64)value, capital);
    if (sz == 4)
        return to_hexstring((u64)value, capital);
    if (sz == 8)
        return to_hexstring((u64)value, capital);
    else return (char*)to_hex_not_supported;
}

#endif
