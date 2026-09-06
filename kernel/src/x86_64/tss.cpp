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
#include <kernel.h>
#include <link_definitions.h>
#include <memory.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <x86_64/tss.h>

#include <print>

namespace TSS {
TSSEntry tssEntry;

void initialize() {
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
    // 0x28 -> offset of TSS in GDT.
    asm volatile(
        "mov $0x28, %%ax\n\t"
        "ltr %%ax\n\t" ::: "rax");

    // Allocate an interrupt kernel stack
    constexpr uintptr_t InterruptStackSizePages = 2;
    constexpr uintptr_t InterruptStackSize = InterruptStackSizePages * PAGE_SIZE;
    constexpr usz interrupt_stack_flags = (usz)Memory::PageTableFlag::Present
                                          | (usz)Memory::PageTableFlag::ReadWrite;
    uintptr_t InterruptStackAddress = Memory::KERNEL_INTERRUPT_STACK_BASE;
    auto physical_stack_base = Memory::request_pages(InterruptStackSizePages);
    if (physical_stack_base == 0) {
        std::print("[ELF]: Couldn't allocate stack for new userspace process (kernel stack)\n");
        return;
    }
    const uintptr_t virtual_stack_base = InterruptStackAddress;
    const uintptr_t virtual_stack_top = virtual_stack_base + InterruptStackSize;
    InterruptStackAddress += InterruptStackSize + PAGE_SIZE;

    Memory::map_pages(
        (void*)virtual_stack_base,
        physical_stack_base,
        interrupt_stack_flags,
        InterruptStackSizePages);

    Memory::unmap((void*)(virtual_stack_base - PAGE_SIZE));
    Memory::unmap((void*)virtual_stack_top);

    memset((void*)virtual_stack_base, 0, InterruptStackSize);
    // Ring N -> Ring 0 transition loads this stack
    tssEntry.set_stack(virtual_stack_top);

    std::print(
        "  Ring 0 Stack: 0x{:016x} (PHYS 0x{:016x})\n",
        virtual_stack_top,
        (uintptr_t)physical_stack_base + InterruptStackSize);

    // Ring 1 and 2 unused by LensorOS
    tssEntry.set_stack(0, TSSEntry::RSP::One);
    tssEntry.set_stack(0, TSSEntry::RSP::Two);

    Scheduler::StartupProcess.kernel_stack_top = virtual_stack_top;

    for (int i = (int)TSSEntry::IST::One; i <= (int)TSSEntry::IST::Seven; ++i) {
        auto physical_base = Memory::request_pages(InterruptStackSizePages);
        if (physical_base == 0) {
            std::print("[TSS]: Couldn't allocate interrupt stack (system may break)\n");
            return;
        }

        // Map in the higher half so all processes share this mapping.
        // Each i gets a guard page following it.
        const int index = i - 1;
        const uintptr_t virtual_offset = index * InterruptStackSize
                                         + index * PAGE_SIZE;
        const uintptr_t virtual_base = Memory::KERNEL_INTERRUPT_STACK_BASE + virtual_offset;
        Memory::map_pages(
            (void*)virtual_base,
            physical_base,
            interrupt_stack_flags,
            InterruptStackSizePages);
        uintptr_t virtual_top = virtual_base + InterruptStackSize;
        // Guard page (stack overflow)
        Memory::unmap((void*)(virtual_base - PAGE_SIZE));
        // Guard page (stack underflow)
        Memory::unmap((void*)virtual_top);

        memset((void*)virtual_base, 0, InterruptStackSize);

        std::print("  IST{}: 0x{:016x} (PHYS 0x{:016x})\n", i, virtual_top, (uintptr_t)physical_base + InterruptStackSize);
        tssEntry.set_ist(virtual_top, (TSSEntry::IST)i);
    }
}
}  // namespace TSS
