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

#include <format>

#include <tss.h>

#include <link_definitions.h>
#include <memory.h>
#include <gdt.h>

TSSEntry tssEntry;
// USED IN `userswitch.asm` `jump_to_userland_function` AS EXTERNAL SYMBOL.
void* tss;

void TSS::initialize() {
    tss = &tssEntry;
    // Zero out TSS entry.
    memset(&tssEntry, 0, sizeof(TSSEntry));
    // Set byte limit of TSS Entry past base address.
    u64 limit = sizeof(TSSEntry) - 1;
    gGDT.TSS.set_limit(limit);
    // Set base address to address of TSS Entry.
    u64 base = V2P((u64)&tssEntry);
    //u64 base = (u64)&tssEntry;
    gGDT.TSS.set_base(base);
    std::print("[TSS]: Initialized\n"
               "  Base:  {:#016x}\n"
               "  Limit: {:#08x}\n"
               , gGDT.TSS.base()
               , gGDT.TSS.limit()
               );
    // Store current stack pointer in TSS entry.
    u64 stackPointer { 0 };
    asm("movq %%rsp, %0\n\t"
        : "=m"(stackPointer)
        );
    tssEntry.set_stack(stackPointer);
    asm("mov $0x28, %%ax\n\t"
        "ltr %%ax\n\t"
        ::: "rax"
        );
}
