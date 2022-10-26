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

#include <interrupts/idt.h>

#include <integers.h>
#include <memory.h>

IDTR gIDT;

IDTR::IDTR(u16 limit, u64 offset)
    : Limit(limit),
      Offset(offset)
{
    // Ensure table is zeroed out before use.
    memset((void*)Offset, 0, Limit);
}

/* Install an interrupt handler within the interrupt descriptor table.
 *   Parameters:
 *     handler_address  --  Memory address of interrupt handler to install in 64-bit unsigned integer format.
 *     entryOffset      --  Vector offset of interrupt that handler will be called on.
 *     typeAttribute    --  Instructs CPU on how to prepare for the interrupt handler to execute.
 *     selector         --  Global Descriptor Table Entry Offset of code segment that handler lies within.
 */
void IDTR::install_handler
(
    u64 handler_address,
    u8 entryOffset,
    u8 typeAttribute,
    u8 selector
 )
{
    IDTEntry* interrupt = (IDTEntry*)(Offset + entryOffset * sizeof(IDTEntry));
    interrupt->SetOffset((u64)handler_address);
    interrupt->TypeAttribute = typeAttribute;
    interrupt->Selector = selector;
}

void IDTEntry::SetOffset(u64 offset) {
    Offset0 = (u16)(offset & 0x000000000000ffff);
    Offset1 = (u16)((offset & 0x00000000ffff0000) >> 16);
    Offset2 = (u32)((offset & 0xffffffff00000000) >> 32);
}

u64 IDTEntry::GetOffset() {
    u64 offset = 0;
    offset |= (u64)Offset0;
    offset |= ((u64)Offset1) << 16;
    offset |= ((u64)Offset2) << 32;
    return offset;
}

