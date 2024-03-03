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
#include <format>
#include <hpet.h>
#include <kernel.h>
#include <keyboard.h>
#include <kstage1.h>
#include <math.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <pit.h>
#include <rtc.h>
#include <scheduler.h>

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
    const RTCData& tm = gRTC.Time;
    u32 startOffset = position.x;
    gRend.puts(position, std::format("Now is {}:{}:{} on {}/{}/{}",
        tm.hour,
        tm.minute,
        tm.second,
        tm.date,
        tm.month,
        tm.year));
    gRend.crlf(position, startOffset);
}

extern "C" void kmain(BootInfo* bInfo) {
    // The heavy lifting is done within the kstage1 function.
    kstage1(bInfo);
    // I'm lovin' it :^) (Plays Maccy's theme).
    constexpr usz MACCYS_BPM = 125;
    constexpr usz MACCYS_STEP_LENGTH_MILLISECONDS = (60 * 1000 / MACCYS_BPM) / 4;
    gPIT.play_sound(262, MACCYS_STEP_LENGTH_MILLISECONDS); // C4
    gPIT.play_sound(294, MACCYS_STEP_LENGTH_MILLISECONDS); // D4
    gPIT.wait();                                           // Rest
    gPIT.play_sound(330, MACCYS_STEP_LENGTH_MILLISECONDS); // E4
    gPIT.wait();                                           // Rest
    gPIT.play_sound(440, MACCYS_STEP_LENGTH_MILLISECONDS); // A4
    gPIT.wait();                                           // Rest
    gPIT.play_sound(392, MACCYS_STEP_LENGTH_MILLISECONDS); // G4

    // Tasks that need done by a kernel thread and done frequently should go
    // in this loop.
    for (;;) {
        // Free pages which previously housed page maps (or portions thereof).
        if (Scheduler::PageMapsToFree.size() > 1) {
            // TODO: Abstract x86_64
            // Disable interrupts; we do this to prevent a timer interrupt causing a
            // yield away from this thread, which could invalidate the iterator in the
            // following loop.
            asm ("cli");
            //std::print("[KERNEL]: Disabled interrupts; freeing {} page tables\n", Scheduler::PageMapsToFree.size());

            for (Memory::PageTable* table : Scheduler::PageMapsToFree) {
                //std::print("[KERNEL]: Freeing page table at {}\n", (void*)table);
                Memory::free_page_map(table);
            }

            // Once we free a page map, we no longer need to free it.
            Scheduler::PageMapsToFree.clear();

            // TODO: Abstract x86_64
            // Enable interrupts (allow yielding away as it now won't cause iterator invalidation or anything)
            //std::print("[KERNEL]: Enabling interrupts after freeing page tables\n");
            asm ("sti");
        }
    }

    // KERNEL INACTIVE
    hang();
}
