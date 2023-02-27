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
#include <memory.h>

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

#endif
