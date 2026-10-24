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

#ifndef LENSOR_OS_UART_H
#define LENSOR_OS_UART_H

#include <integers.h>

#include <string_view>

#define BAUD_FREQ 115200
#define BAUD_RATE 9600
#define BAUD_DIVISOR (BAUD_FREQ / BAUD_RATE)

// TODO: Add capability for selecting communication channel (COM1, COM2, etc).
namespace UART {
void initialize();
bool initialized();

enum class Chip;
const char* get_uart_chip_name(Chip);

// Called from the UART interrupt handler when data is received.
u8 read();

// Write a singular byte verbatim to serial output.
void out(u8);
inline void outc(char c) {
    out((u8)c);
}

// Write a c-style null-terminated string to serial output.
void out(std::string_view str);
// Write a number of bytes from a given buffer to serial output.
void out(const u8* buffer, u64 numberOfBytes);
// Write the given number as a string to serial output.
void out(u64);
void out(u32);
void out(u16);

}  // namespace UART

#endif  // LENSOR_OS_UART_H
