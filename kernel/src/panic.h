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


#ifndef LENSOR_OS_PANIC_H
#define LENSOR_OS_PANIC_H

#define PanicStartX 400
#define PanicStartY 500

class InterruptFrame;
class InterruptFrameError;

__attribute__((no_caller_saved_registers))
void panic(const char* panicMessage);

__attribute__((no_caller_saved_registers))
void panic(InterruptFrame*, const char* panicMessage);

__attribute__((no_caller_saved_registers))
void panic(InterruptFrameError*, const char* panicMessage);

#define ASSERT(cond, msg) if (!(cond)) panic("Assertion \"" #cond "\" failed: " msg)
#define UNREACHABLE(msg) ASSERT(0, "UNREACHABLE  " msg); hang()

[[noreturn]]
void hang();

#endif
