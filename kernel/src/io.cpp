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

#include <io.h>
#include <integers.h>

void out8(u16 port, u8 value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void out16(u16 port, u16 value) {
    asm volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

void out32(u16 port, u32 value) {
    asm volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

u8 in8(u16 port) {
    u8 retValue;
    asm volatile ("inb %1, %0" : "=a"(retValue) : "Nd"(port));
    return retValue;
}

u16 in16(u16 port) {
    u16 retValue;
    asm volatile ("inw %1, %0" : "=a"(retValue) : "Nd"(port));
    return retValue;
}

u32 in32(u16 port) {
    u32 retValue;
    asm volatile ("inl %1, %0" : "=a"(retValue) : "Nd"(port));
    return retValue;
}

void io_wait() {
    // Port 0x80 -- Unused port that is safe to read/write
    asm volatile ("outb %%al, $0x80" : : "a"(0));
}
