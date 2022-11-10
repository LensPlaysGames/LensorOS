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

#include <kernel.h>

#include <basic_renderer.h>
#include <boot.h>
#include <cstr.h>
#include <debug.h>
#include <hpet.h>
#include <interrupts/interrupts.h>
#include <keyboard.h>
#include <kstage1.h>
#include <math.h>
#include <memory/common.h>
#include <memory/heap.h>
#include <memory/virtual_memory_manager.h>
#include <memory/physical_memory_manager.h>
#include <pit.h>
#include <rtc.h>
#include <string.h>
#include <tss.h>
#include <uart.h>

void print_memory_info(Vector2<u64>& position) {
    u32 startOffset = position.x;
    u64 totalRAM = Memory::total_ram();
    u64 freeRAM = Memory::free_ram();
    u64 usedRAM = Memory::used_ram();
    gRend.puts(position, "Memory Info:");
    gRend.crlf(position, startOffset);
    gRend.puts(position, "|- Total RAM: ");
    gRend.puts(position, to_string(TO_MiB(totalRAM)));
    gRend.puts(position, " MiB (");
    gRend.puts(position, to_string(TO_KiB(totalRAM)));
    gRend.puts(position, " KiB)");
    gRend.crlf(position, startOffset);
    gRend.puts(position, "|- Free RAM: ");
    gRend.puts(position, to_string(TO_MiB(freeRAM)));
    gRend.puts(position, " MiB (");
    gRend.puts(position, to_string(TO_KiB(freeRAM)));
    gRend.puts(position, " KiB)");
    gRend.crlf(position, startOffset);
    gRend.puts(position, "`- Used RAM: ");
    gRend.puts(position, to_string(TO_MiB(usedRAM)));
    gRend.puts(position, " MiB (");
    gRend.puts(position, to_string(TO_KiB(usedRAM)));
    gRend.puts(position, " KiB)");
    gRend.crlf(position, startOffset);
}

void print_now(Vector2<u64>& position) {
    const RTCData& tm = gRTC.Time;
    u32 startOffset = position.x;
    gRend.puts(position, "Now is ");
    gRend.puts(position, to_string(tm.hour));
    gRend.putchar(position, ':');
    gRend.puts(position, to_string(tm.minute));
    gRend.putchar(position, ':');
    gRend.puts(position, to_string(tm.second));
    gRend.puts(position, " on ");
    gRend.puts(position, to_string(tm.year));
    gRend.putchar(position, '-');
    gRend.puts(position, to_string(tm.month));
    gRend.putchar(position, '-');
    gRend.puts(position, to_string(tm.date));
    gRend.crlf(position, startOffset);
}

extern "C" void kmain(BootInfo* bInfo) {
    // The heavy lifting is done within the kstage1 function.
    kstage1(bInfo);
    std::print("\n\033[1;33m!===--- You have now booted into LensorOS ---===!\033[0m\n");
    // Clear + swap screen (ensure known state: blank).
    gRend.clear(0x00000000);
    gRend.swap();
    // GPLv3 LICENSE REQUIREMENT (interactive terminal must print copyright notice).
    const char* GPLv3 = "LensorOS  Copyright (C) 2022, Contributors To LensorOS.";
    // TO SERIAL
    std::print("{}\n\n", GPLv3);
    // TO SCREEN
    Vector2<u64> drawPosition = { 0, 0 };
    gRend.BackgroundColor = 0xffffffff;
    gRend.puts(drawPosition, GPLv3, 0x00000000);
    gRend.BackgroundColor = 0x00000000;
    gRend.crlf(drawPosition);
    // END GPLv3 LICENSE REQUIREMENT.
    gRend.puts(drawPosition, "Do a barrel roll!");
    gRend.crlf(drawPosition);
    gRend.swap({0, 0}, {80000, gRend.Font->PSF1_Header->CharacterSize * 2u});

    // I'm lovin' it :^) (Plays Maccy's theme).
    constexpr double MACCYS_BPM = 125;
    constexpr double MACCYS_STEP_LENGTH_SECONDS = (60 / MACCYS_BPM) / 4;
    gPIT.play_sound(262, MACCYS_STEP_LENGTH_SECONDS); // C4
    gPIT.play_sound(294, MACCYS_STEP_LENGTH_SECONDS); // D4
    gPIT.wait();                                      // Rest
    gPIT.play_sound(330, MACCYS_STEP_LENGTH_SECONDS); // E4
    gPIT.wait();                                      // Rest
    gPIT.play_sound(440, MACCYS_STEP_LENGTH_SECONDS); // A4
    gPIT.wait();                                      // Rest
    gPIT.play_sound(392, MACCYS_STEP_LENGTH_SECONDS); // G4

    // Start keyboard input at draw position, not origin.
    Keyboard::gText.set_cursor_from_pixel_position(drawPosition);

    u32 debugInfoX = gRend.Target->PixelWidth - 300;
    while (true) {
        drawPosition = {debugInfoX, 0};
        // PRINT REAL TIME
        gRTC.update_data();
        print_now(drawPosition);
        gRend.crlf(drawPosition, debugInfoX);
        // PRINT PIT ELAPSED TIME.
        gRend.puts(drawPosition, "PIT Elapsed: ");
        gRend.puts(drawPosition, to_string(gPIT.seconds_since_boot()));
        gRend.crlf(drawPosition, debugInfoX);
        // PRINT RTC ELAPSED TIME.
        gRend.puts(drawPosition, "RTC Elapsed: ");
        gRend.puts(drawPosition, to_string(gRTC.seconds_since_boot()));
        gRend.crlf(drawPosition, debugInfoX);
        // PRINT HPET ELAPSED TIME.
        gRend.puts(drawPosition, "HPET Elapsed: ");
        gRend.puts(drawPosition, to_string(gHPET.seconds()));
        gRend.crlf(drawPosition, debugInfoX);
        // PRINT MEMORY INFO.
        gRend.crlf(drawPosition, debugInfoX);
        print_memory_info(drawPosition);
        // UPDATE TOP RIGHT CORNER OF SCREEN.
        gRend.swap({debugInfoX, 0}, {80000, 400});
    }
    // HALT LOOP (KERNEL INACTIVE).
    while (true)
        asm ("hlt");
}
