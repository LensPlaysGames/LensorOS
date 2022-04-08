#include <kernel.h>

#include <basic_renderer.h>
#include <boot.h>
#include <cstr.h>
#include <hpet.h>
#include <interrupts/interrupts.h>
#include <keyboard.h>
#include <kstage1.h>
#include <memory/common.h>
#include <memory/physical_memory_manager.h>
#include <pit.h>
#include <rtc.h>
#include <tss.h>
#include <uart.h>

void print_memory_info() {
    u32 startOffset = gRend.DrawPos.x;
    u64 totalRAM = Memory::get_total_ram();
    u64 freeRAM = Memory::get_free_ram();
    u64 usedRAM = Memory::get_used_ram();
    gRend.puts("Memory Info:");
    gRend.crlf(startOffset);
    gRend.puts("|- Total RAM: ");
    gRend.puts(to_string(TO_MiB(totalRAM)));
    gRend.puts(" MiB (");
    gRend.puts(to_string(TO_KiB(totalRAM)));
    gRend.puts(" KiB)");
    gRend.crlf(startOffset);
    gRend.puts("|- Free RAM: ");
    gRend.puts(to_string(TO_MiB(freeRAM)));
    gRend.puts(" MiB (");
    gRend.puts(to_string(TO_KiB(freeRAM)));
    gRend.puts(" KiB)");
    gRend.crlf(startOffset);
    gRend.puts("`- Used RAM: ");
    gRend.puts(to_string(TO_MiB(usedRAM)));
    gRend.puts(" MiB (");
    gRend.puts(to_string(TO_KiB(usedRAM)));
    gRend.puts(" KiB)");
    gRend.crlf(startOffset);
}

void print_now(u64 xOffset = 0) {
    RTCData& tm = gRTC.Time;
    gRend.puts("Now is ");
    gRend.puts(to_string(tm.hour));
    gRend.putchar(':');
    gRend.puts(to_string(tm.minute));
    gRend.putchar(':');
    gRend.puts(to_string(tm.second));
    gRend.puts(" on ");
    gRend.puts(to_string(tm.year));
    gRend.putchar('-');
    gRend.puts(to_string(tm.month));
    gRend.putchar('-');
    gRend.puts(to_string(tm.date));
    gRend.crlf(xOffset);
}

void test_userland_function() {
    for (;;) {
        asm volatile ("mov $0, %rax\r\n\t"
                      "int $0x80\r\n\t");
    }
}

extern "C" void kmain(BootInfo* bInfo) {
    // The heavy lifting is done within the `kernel_init` function (found in `kUtility.cpp`).
    kstage1(bInfo);
    UART::out("\r\n\033[1;33m!===--- You have now booted into LensorOS ---===!\033[0m\r\n");
    //// Clear + swap screen (ensure known state: blank).
    gRend.clear(0x00000000);
    gRend.swap();
    ///// GPLv3 LICENSE REQUIREMENT (interactive terminal must print copyright notice).
    const char* GPLv3 = "<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>";
    //// TO SERIAL
    UART::out(GPLv3);
    UART::out("\r\n\r\n");
    //// TO SCREEN
    gRend.BackgroundColor = 0xffffffff;
    gRend.puts(GPLv3, 0x00000000);
    gRend.BackgroundColor = 0x00000000;
    gRend.crlf();
    gRend.swap({0, 0}, {80000, gRend.Font->PSF1_Header->CharacterSize});
    /// END GPLv3 LICENSE REQUIREMENT.

    // USERLAND SWITCH TESTING
    // jump_to_userland_function((void*)test_userland_function);

    // FIXME FIXME FIXME
    // NULL DE-REFERENCE TESTING
    //UART::out("Going to null de-reference\r\n");
    //u8* badPtr = nullptr;
    //u8 dereferenced = *badPtr;
    //(void)dereferenced;
    //UART::out("Null de-referenced!\r\n");

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
    Keyboard::gText.set_cursor_from_pixel_position(gRend.DrawPos);

    u32 debugInfoX = gRend.Target->PixelWidth - 300;
    while (true) {
        gRend.DrawPos = {debugInfoX, 0};
        // PRINT REAL TIME
        gRTC.update_data();
        print_now(debugInfoX);
        gRend.crlf(debugInfoX);
        // PRINT PIT ELAPSED TIME.
        gRend.puts("PIT Elapsed: ");
        gRend.puts(to_string(gPIT.seconds_since_boot()));
        gRend.crlf(debugInfoX);
        // PRINT RTC ELAPSED TIME.
        gRend.puts("RTC Elapsed: ");
        gRend.puts(to_string(gRTC.seconds_since_boot()));
        gRend.crlf(debugInfoX);
        // PRINT HPET ELAPSED TICKS.
        gRend.puts("HPET Elapsed: ");
        gRend.puts(to_string(gHPET.get_seconds()));
        gRend.crlf(debugInfoX);
        // PRINT MEMORY INFO.
        gRend.crlf(debugInfoX);
        print_memory_info();
        // UPDATE TOP RIGHT CORNER OF SCREEN.
        gRend.swap({debugInfoX, 0}, {80000, 400});
    }

    // HALT LOOP (KERNEL INACTIVE).
    while (true)
        asm ("hlt");
}
