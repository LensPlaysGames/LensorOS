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

#ifndef LENSOR_OS_INTERRUPTS_H
#define LENSOR_OS_INTERRUPTS_H

#include <integers.h>

#ifdef x86_64
#define IRQ_VECTOR_OFFSET 0x20
#else
#define IRQ_VECTOR_OFFSET 0
#endif

extern "C" void end_of_interrupt(u8 irq);
void enable_interrupt(u8 irq);
void install_interrupt_handler(void* handler, u8 vector);

#endif  // LENSOR_OS_INTERRUPTS_H
