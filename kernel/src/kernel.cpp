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
#include <memory/physical_memory_manager.h>
#include <pit.h>
#include <rtc.h>

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
    for (;;) {
        //std::print("DRAWING\n");
        drawPosition = {debugInfoX, 0};
        // PRINT REAL TIME
        gRTC.update_data();
        print_now(drawPosition);
        gRend.crlf(drawPosition, debugInfoX);
        // PRINT PIT ELAPSED TIME.
        gRend.puts(drawPosition, std::format("PIT Elapsed: {:.3}", gPIT.seconds_since_boot()));
        gRend.crlf(drawPosition, debugInfoX);
        // PRINT RTC ELAPSED TIME.
        gRend.puts(drawPosition, std::format("RTC Elapsed: {:.3}", gRTC.seconds_since_boot()));
        gRend.crlf(drawPosition, debugInfoX);
        // PRINT HPET ELAPSED TIME.
        gRend.puts(drawPosition, std::format("HPET Elapsed: {:.3}", gHPET.seconds()));
        gRend.crlf(drawPosition, debugInfoX);
        // PRINT MEMORY INFO.
        gRend.crlf(drawPosition, debugInfoX);
        print_memory_info(drawPosition);
        // UPDATE TOP RIGHT CORNER OF SCREEN.
        gRend.swap({debugInfoX, 0}, {80000, 400});

        // Idle until next interrupt.
        // FIXME: This doesn’t work because it causes the keyboard to become unresponsive.
        //asm volatile ("hlt");
    }

    // HALT LOOP (KERNEL INACTIVE).
    for (;;) asm volatile ("hlt");
}
