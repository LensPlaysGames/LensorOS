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
    constexpr size_t KernelInterruptStackSizePages = 2;
    constexpr size_t KernelInterruptStackSize = KernelInterruptStackSizePages * PAGE_SIZE;
    // FIXME: Utilize higher half mapping??? P2V()?
    auto physical_stack_base = Memory::request_pages(KernelInterruptStackSizePages);
    if (physical_stack_base == 0) {
        std::print("[ELF]: Couldn't allocate stack for new userspace process (kernel stack)\n");
        return;
    }
    memset(physical_stack_base, 0, KernelInterruptStackSize);
    auto physical_stack_top = ((uintptr_t)physical_stack_base) + KernelInterruptStackSize;
    std::print("  Stack: 0x{:016x}\n", physical_stack_top);

    tssEntry.set_stack(physical_stack_top);
    Scheduler::StartupProcess.kernel_stack_top = physical_stack_top;

    constexpr usz interrupt_stack_flags = (usz)Memory::PageTableFlag::Present
                                          | (usz)Memory::PageTableFlag::ReadWrite;
    for (int i = (int)TSSEntry::IST::One; i <= (int)TSSEntry::IST::Seven; ++i) {
        constexpr uintptr_t InterruptStackSizePages = 2;
        constexpr uintptr_t InterruptStackSize = InterruptStackSizePages * PAGE_SIZE;

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

        std::print("Setting IST{} to 0x{:016x}\n", i, virtual_top);
        tssEntry.set_ist(virtual_top, (TSSEntry::IST)i);
    }
}
}  // namespace TSS
