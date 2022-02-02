#include "kUtility.h"

// TODO:
// - Set up a barebones TSS with an ESP0 stack.
// - Port kernel to a new build system (as many as possible).
// - Abstract `timer` class (namespace?) that will be used for an API for things like `sleep`
// - Read more of this: https://pages.cs.wisc.edu/~remzi/OSTEP/
// - Save parsed PCI devices for quick lookup (device tree).
// - FILE SYSTEM:
//   - Virtual File System that will store intermediate representation of files/folders/worlds/storage media devices
//   - AHCI Driver Update: DMA ATA Write implementation
//   - Another filesystem better suited for mass storage (Ext2? Proprietary?)
// - Write ASM interrupt wrapper (no longer rely on `__attribute__((interrupt))`)
//   - See James Molloy's tutorials for an example: http://www.jamesmolloy.co.uk/tutorial_html/
// - Move includes to forward declarations where possible, move includes from `.h` to `.cpp`
// - Add GPLv3 license header to top of every source file (exactly as seen in LICENSE).

void print_memory_info() {
    u32 startOffset = gRend.DrawPos.x;
    gRend.crlf(startOffset);
    gRend.putstr("Memory Info:");
    gRend.crlf(startOffset);
    gRend.putstr("|\\");
    gRend.crlf(startOffset);
    gRend.putstr("| Free RAM: ");
    gRend.putstr(to_string(gAlloc.get_free_ram() / 1024));
    gRend.putstr(" KiB (");
    gRend.putstr(to_string(gAlloc.get_free_ram() / 1024 / 1024));
    gRend.putstr(" MiB)");
    gRend.crlf(startOffset);
    gRend.putstr("|\\");
    gRend.crlf(startOffset);
    gRend.putstr("| Used RAM: ");
    gRend.putstr(to_string(gAlloc.get_used_ram() / 1024));
    gRend.putstr(" KiB (");
    gRend.putstr(to_string(gAlloc.get_used_ram() / 1024 / 1024));
    gRend.putstr(" MiB)");
    gRend.crlf(startOffset);
    gRend.putstr(" \\");
    gRend.crlf(startOffset);
    gRend.putstr("  Reserved RAM: ");
    gRend.putstr(to_string(gAlloc.get_reserved_ram() / 1024));
    gRend.putstr(" KiB (");
    gRend.putstr(to_string(gAlloc.get_reserved_ram() / 1024 / 1024));
    gRend.putstr(" MiB)");
    gRend.crlf(startOffset);
}

void print_now(u64 xOffset = 0) {
    gRend.crlf(xOffset);
    gRend.putstr("Now is ");
    gRend.putstr(to_string((u64)gRTC.Time.hour));
    gRend.putchar(':');
    gRend.putstr(to_string((u64)gRTC.Time.minute));
    gRend.putchar(':');
    gRend.putstr(to_string((u64)gRTC.Time.second));
    gRend.putstr(" on ");
    gRend.putstr(to_string((u64)gRTC.Time.year));
    gRend.putchar('-');
    gRend.putstr(to_string((u64)gRTC.Time.month));
    gRend.putchar('-');
    gRend.putstr(to_string((u64)gRTC.Time.date));
    gRend.crlf(xOffset);
}

void srl_memory_info() {
    srl.writestr("\r\n");
    srl.writestr("Memory Info:");
    srl.writestr("\r\n");
    srl.writestr("|\\");
    srl.writestr("\r\n");
    srl.writestr("| Free RAM: ");
    srl.writestr(to_string(gAlloc.get_free_ram() / 1024));
    srl.writestr(" KiB (");
    srl.writestr(to_string(gAlloc.get_free_ram() / 1024 / 1024));
    srl.writestr(" MiB)");
    srl.writestr("\r\n");
    srl.writestr("|\\");
    srl.writestr("\r\n");
    srl.writestr("| Used RAM: ");
    srl.writestr(to_string(gAlloc.get_used_ram() / 1024));
    srl.writestr(" KiB (");
    srl.writestr(to_string(gAlloc.get_used_ram() / 1024 / 1024));
    srl.writestr(" MiB)");
    srl.writestr("\r\n");
    srl.writestr(" \\");
    srl.writestr("\r\n");
    srl.writestr("  Reserved RAM: ");
    srl.writestr(to_string(gAlloc.get_reserved_ram() / 1024));
    srl.writestr(" KiB (");
    srl.writestr(to_string(gAlloc.get_reserved_ram() / 1024 / 1024));
    srl.writestr(" MiB)");
    srl.writestr("\r\n");
}

extern "C" void _start(BootInfo* bInfo) {
    // The heavy lifting is done within the `kernel_init` function (found in `kUtility.cpp`).
    kernel_init(bInfo);
    srl.writestr("!===--- You have now booted into LensorOS ---===!\r\n");
    // Clear screen (ensure known state).
    gRend.clear();
    /// GPLv3 LICENSE REQUIREMENT (interactive terminal must print copyright notice).
    const char* GPLv3 = "<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>";
    // TO SERIAL
    srl.writestr(GPLv3);
    srl.writestr("\r\n");
    // TO SCREEN
    gRend.BackgroundColor = 0xffffffff;
    gRend.putstr(GPLv3, 0x00000000);
    gRend.BackgroundColor = 0x00000000;
    gRend.crlf();
    gRend.swap();
    /// END GPLv3 LICENSE REQUIREMENT.
    // Start keyboard input at draw position, not origin.
    gTextPosition = gRend.DrawPos; 
    while (true) {
        // PRINT PIT ELAPSED TIME.
        gRend.DrawPos = {500, 0};
        gRend.putstr("PIT Elapsed: ");
        gRend.putstr(to_string(gPIT.seconds_since_boot()));
        gRend.crlf();
        // PRINT REAL TIME.
        gRTC.update_data();
        print_now(500);
        // PRINT RTC ELAPSED TIME.
        gRend.putstr("It has been ");
        gRend.putstr(to_string(gRTC.seconds_since_boot()));
        gRend.putstr(" seconds since boot");
        gRend.crlf(500);
        // PRINT MEMORY INFO.
        print_memory_info();
        // UPDATE TOP RIGHT OF SCREEN ONLY.
        gRend.swap({500, 0}, {80000, 400});
    }
    // HALT LOOP (KERNEL INACTIVE).
    while (true) {
        asm ("hlt");
    }
}
