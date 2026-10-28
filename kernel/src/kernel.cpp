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

#include <basic_renderer.h>
#include <boot.h>
#include <cstr.h>
#include <kernel.h>
#include <keyboard.h>
#include <kstage1.h>
#include <math.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <time.h>

#include <print>

#if defined(x86_64)
#define DISABLE_INTERRUPTS asm volatile("cli")
#define ENABLE_INTERRUPTS asm volatile("sti")
#elif defined(aarch64)
#define DISABLE_INTERRUPTS                                     \
    uint64_t __flags;                                          \
    asm volatile("mrs %0, daif" : "=r"(__flags) : : "memory"); \
    asm volatile("msr daifset, #3" : : : "memory");
#define ENABLE_INTERRUPTS \
    asm volatile("msr daif, %0" : : "r"(__flags) : "memory");
#endif

void print_memory_info(Vector2<u64>& position) {
    u32 startOffset = position.x;
    u64 totalRAM = Memory::total_ram();
    u64 freeRAM = Memory::free_ram();
    u64 usedRAM = Memory::used_ram();
    gRend.puts(position, std::format("Memory Info: {}", startOffset));
    gRend.crlf(position, startOffset);
    gRend.puts(position, std::format("|- Total RAM: {} MiB ({} KiB)", TO_MiB(totalRAM), TO_KiB(totalRAM)));
    gRend.crlf(position, startOffset);
    gRend.puts(position, std::format("|- Free RAM: {} MiB ({} KiB)", TO_MiB(freeRAM), TO_KiB(freeRAM)));
    gRend.crlf(position, startOffset);
    gRend.puts(position, std::format("`- Used RAM: {} MiB ({} KiB)", TO_MiB(usedRAM), TO_KiB(usedRAM)));
    gRend.crlf(position, startOffset);
}

void print_now(Vector2<u64>& position) {
    Time::tm now{};
    Time::mktime(&now);
    u32 startOffset = position.x;
    gRend.puts(
        position,
        std::format(
            "Now is {}:{}:{} on {}/{}/{}",
            now.hours,
            now.minutes,
            now.seconds,
            now.day_of_month,
            now.month + 1,
            now.years_since_1900 + 1900));
    gRend.crlf(position, startOffset);
}

extern "C" void kregulate() {
    std::print("kregulate()\n");
    // Tasks that need done by a kernel thread and done frequently should go
    // in this loop.
    for (;;) {
        // Free pages which previously housed page maps (or portions thereof).
        if (Scheduler::PageMapsToFree.size() > 15) {
            // Disable interrupts; we do this to prevent a timer interrupt causing a
            // yield away from this thread, which could invalidate the iterator in the
            // following loop.
            DISABLE_INTERRUPTS;
            std::print("[KERNEL]: Disabled interrupts; freeing {} page tables\n", Scheduler::PageMapsToFree.size());

            for (Memory::PageTable* table : Scheduler::PageMapsToFree) {
                // std::print("[KERNEL]: Freeing page table at {}\n", (void*)table);
                Memory::free_page_map(table);
            }

            // Once we free a page map, we no longer need to free it.
            Scheduler::PageMapsToFree.clear();

            // Enable interrupts (allow yielding away as it now won't cause iterator invalidation or anything)
            std::print("[KERNEL]: Enabling interrupts after freeing page tables\n");
            ENABLE_INTERRUPTS;
        }

        Scheduler::yield();
    }
}

extern "C" void kmain(uintptr_t base, BootInfo* bInfo) {
    // The heavy lifting is done within the kstage1 function.
    kstage1(base, bInfo);

    std::print("kmain()\n");

    kregulate();

    // KERNEL INACTIVE
    hang();
}
