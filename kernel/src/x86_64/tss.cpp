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

#include <gdt.h>
#include <link_definitions.h>
#include <memory.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <x86_64/tss.h>

#include <print>

// USED IN `userswitch.asm` `jump_to_userland_function` AS EXTERNAL SYMBOL.
void* tss;

namespace TSS {
TSSEntry tssEntry;

void initialize() {
    tss = &tssEntry;
    // Zero out TSS entry.
    memset(&tssEntry, 0, sizeof(TSSEntry));
    // Set byte limit of TSS Entry past base address.
    u64 limit = sizeof(TSSEntry) - 1;
    gGDT.TSS.set_limit(limit);
    // Set base address to address of TSS Entry.
    u64 base = V2P((u64)&tssEntry);
    gGDT.TSS.set_base(base);
    std::print(
        "[TSS]: Initialized\n"
        "  Base:  {:#016x}\n"
        "  Limit: {:#08x}\n",
        gGDT.TSS.base(),
        gGDT.TSS.limit());
    // 0x28 -> offset of TSS in GDT (I think).
    asm volatile(
        "mov $0x28, %%ax\n\t"
        "ltr %%ax\n\t" ::: "rax");

    // Allocate an interrupt kernel stack
    constexpr size_t stack_flags = (size_t)Memory::PageTableFlag::Present
                                   | (size_t)Memory::PageTableFlag::ReadWrite;
    constexpr size_t KernelInterruptStackPages = 2;
    constexpr size_t KernelInterruptStackSize = KernelInterruptStackPages * PAGE_SIZE;
    // FIXME: why this address
    constexpr uintptr_t KernelInterruptStackBase = 0xfffff77f00f00000;
    constexpr uintptr_t KernelInterruptStackTop = KernelInterruptStackBase + KernelInterruptStackSize;
    for (size_t i = 0; i < KernelInterruptStackPages; ++i) {
        auto physical_page = Memory::request_page();
        auto virtual_page = (void*)(KernelInterruptStackBase + (i * PAGE_SIZE));
        Memory::map(virtual_page, physical_page, stack_flags);
    }
    std::print("  Stack: 0x{:016x}\n", KernelInterruptStackTop);
    Scheduler::StartupProcess.kernel_stack = KernelInterruptStackTop;
    tssEntry.set_stack(KernelInterruptStackTop);
}
}  // namespace TSS
