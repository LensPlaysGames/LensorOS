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

#ifndef LENSOR_OS_X86_64_CPU_H
#define LENSOR_OS_X86_64_CPU_H

#include <integers.h>
#include <interrupts/interrupts.h>

struct CPUState {
    u64 RSP;
    u64 RBX;
    u64 RCX;
    u64 RDX;
    u64 RSI;
    u64 RDI;
    u64 RBP;
    u64 R8;
    u64 R9;
    u64 R10;
    u64 R11;
    u64 R12;
    u64 R13;
    u64 R14;
    u64 R15;
    u64 FS;
    u64 GS;
    u64 RAX;
    InterruptFrame Frame;
} __attribute__((packed));

#endif // LENSOR_OS_X86_64_CPU_H
