#include "kUtility.h"

#include "paging/page_frame_allocator.h"

#include "basic_renderer.h"
#include "uart.h"

#include "keyboard.h"
#include "pit.h"
#include "rtc.h"

#include "interrupts/interrupts.h"

// TODO:
// - Set up a barebones TSS with an ESP0 stack.
//   - Each task needs three things:
//     - Task Execution Space, or TES (CS, SS for each privilege level, and one or more DS).
//     - Task-state Segment, or TSS (Segments that make up task execution space, storage for task-state info).
//     - Page Map Level 4 (Loaded into CR3)
//   - Each task is identified by segment selector for it's TSS.
//   - See "Section 7.1.1: Task Structure" of the Intel Software Manual, Volume 3-A.
// - TLB (Translation Lookaside Buffer)
// - Abstract `timer` class (namespace?) that will be used for an API for things like `sleep`
// - Read more of this: https://pages.cs.wisc.edu/~remzi/OSTEP/
// - Save parsed PCI devices for quick lookup (device tree).
// - A slab-style memory allocator.
// - FILE SYSTEM:
//   - Virtual File System that will store intermediate representation of files/folders/worlds/storage media devices
//   - AHCI Driver Update: DMA ATA Write implementation
//   - Another filesystem better suited for mass storage (Ext2? Proprietary?)
// - Write ASM interrupt wrapper (no longer rely on `__attribute__((interrupt))`)
//   - See James Molloy's tutorials for an example: http://www.jamesmolloy.co.uk/tutorial_html/
// - Move includes to forward declarations where possible, move includes from `.h` to `.cpp`
// - Implement actually useful system calls
// - Add GPLv3 license header to top of every source file (exactly as seen in LICENSE).

void print_memory_info() {
    u32 startOffset = gRend.DrawPos.x;
    gRend.puts("Memory Info:");
    gRend.crlf(startOffset);
    gRend.puts("|\\");
    gRend.crlf(startOffset);
    gRend.puts("| Free RAM: ");
    gRend.puts(to_string(gAlloc.get_free_ram() / 1024));
    gRend.puts(" KiB (");
    gRend.puts(to_string(gAlloc.get_free_ram() / 1024 / 1024));
    gRend.puts(" MiB)");
    gRend.crlf(startOffset);
    gRend.puts("|\\");
    gRend.crlf(startOffset);
    gRend.puts("| Used RAM: ");
    gRend.puts(to_string(gAlloc.get_used_ram() / 1024));
    gRend.puts(" KiB (");
    gRend.puts(to_string(gAlloc.get_used_ram() / 1024 / 1024));
    gRend.puts(" MiB)");
    gRend.crlf(startOffset);
    gRend.puts(" \\");
    gRend.crlf(startOffset);
    gRend.puts("  Reserved RAM: ");
    gRend.puts(to_string(gAlloc.get_reserved_ram() / 1024));
    gRend.puts(" KiB (");
    gRend.puts(to_string(gAlloc.get_reserved_ram() / 1024 / 1024));
    gRend.puts(" MiB)");
    gRend.crlf(startOffset);
}

void print_now(u64 xOffset = 0) {
    gRend.puts("Now is ");
    gRend.puts(to_string(gRTC.Time.hour));
    gRend.putchar(':');
    gRend.puts(to_string(gRTC.Time.minute));
    gRend.putchar(':');
    gRend.puts(to_string(gRTC.Time.second));
    gRend.puts(" on ");
    gRend.puts(to_string(gRTC.Time.year));
    gRend.putchar('-');
    gRend.puts(to_string(gRTC.Time.month));
    gRend.putchar('-');
    gRend.puts(to_string(gRTC.Time.date));
    gRend.crlf(xOffset);
}

void srl_memory_info() {
    srl->writestr("\r\n");
    srl->writestr("Memory Info:");
    srl->writestr("\r\n");
    srl->writestr("|\\");
    srl->writestr("\r\n");
    srl->writestr("| Free RAM: ");
    srl->writestr(to_string(gAlloc.get_free_ram() / 1024));
    srl->writestr(" KiB (");
    srl->writestr(to_string(gAlloc.get_free_ram() / 1024 / 1024));
    srl->writestr(" MiB)");
    srl->writestr("\r\n");
    srl->writestr("|\\");
    srl->writestr("\r\n");
    srl->writestr("| Used RAM: ");
    srl->writestr(to_string(gAlloc.get_used_ram() / 1024));
    srl->writestr(" KiB (");
    srl->writestr(to_string(gAlloc.get_used_ram() / 1024 / 1024));
    srl->writestr(" MiB)");
    srl->writestr("\r\n");
    srl->writestr(" \\");
    srl->writestr("\r\n");
    srl->writestr("  Reserved RAM: ");
    srl->writestr(to_string(gAlloc.get_reserved_ram() / 1024));
    srl->writestr(" KiB (");
    srl->writestr(to_string(gAlloc.get_reserved_ram() / 1024 / 1024));
    srl->writestr(" MiB)");
    srl->writestr("\r\n");
}

extern "C" void _start(BootInfo* bInfo) {
    // The heavy lifting is done within the `kernel_init` function (found in `kUtility.cpp`).
    kernel_init(bInfo);
    srl->writestr("\r\n\033[30;47m!===--- You have now booted into LensorOS ---===!\033[0m\r\n");
    // Clear + swap screen (ensure known state: blank).
    gRend.clear(0x00000000);
    gRend.swap();
    /// GPLv3 LICENSE REQUIREMENT (interactive terminal must print copyright notice).
    const char* GPLv3 = "<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>";
    // TO SERIAL
    srl->writestr(GPLv3);
    srl->writestr("\r\n\r\n");
    // TO SCREEN
    gRend.BackgroundColor = 0xffffffff;
    gRend.puts(GPLv3, 0x00000000);
    gRend.BackgroundColor = 0x00000000;
    gRend.crlf();
    gRend.swap({0, 0}, {80000, gRend.Font->PSF1_Header->CharacterSize});
    /// END GPLv3 LICENSE REQUIREMENT.

    // FIXME
    // SYSTEM CALL TESTING
    // Test system call 0 (a test).
    asm volatile ("mov $0, %rax\r\n\t"
                  "int $0x80\r\n\t");
    // Test system call 1 (a test).
    asm volatile ("mov $1, %rax\r\n\t"
                  "int $0x80\r\n\t");
    // Test that invalid system call codes do nothing.
    asm volatile ("mov $2, %rax\r\n\t"
                  "int $0x80\r\n\t");
    asm volatile ("mov $45, %rax\r\n\t"
                  "int $0x80\r\n\t");

    // Start keyboard input at draw position, not origin.
    gTextPosition = gRend.DrawPos;
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
        // PRINT MEMORY INFO.
        gRend.crlf(debugInfoX);
        print_memory_info();
        // UPDATE TOP RIGHT CORNER OF SCREEN.
        gRend.swap({debugInfoX, 0}, {80000, 400});
    }
    
    // HALT LOOP (KERNEL INACTIVE).
    while (true) {
        asm ("hlt");
    }
}
