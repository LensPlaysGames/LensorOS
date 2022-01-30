#include "kUtility.h"

// TODO:
// - Handle utf-8 characters (somehow).
// - Read more of this: https://pages.cs.wisc.edu/~remzi/OSTEP/
// - Save parsed PCI devices for quick lookup (device tree).
// - FILE SYSTEM:
//   - Virtual File System that will store intermediate representation of files/folders/storage media devices
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
	gRend.putstr(" kB (");
	gRend.putstr(to_string(gAlloc.get_free_ram() / 1024 / 1024));
	gRend.putstr(" mB)");
    gRend.crlf(startOffset);
	gRend.putstr("|\\");
    gRend.crlf(startOffset);
	gRend.putstr("| Used RAM: ");
	gRend.putstr(to_string(gAlloc.get_used_ram() / 1024));
	gRend.putstr(" kB (");
	gRend.putstr(to_string(gAlloc.get_used_ram() / 1024 / 1024));
	gRend.putstr(" mB)");
    gRend.crlf(startOffset);
	gRend.putstr(" \\");
    gRend.crlf(startOffset);
	gRend.putstr("  Reserved RAM: ");
	gRend.putstr(to_string(gAlloc.get_reserved_ram() / 1024));
	gRend.putstr(" kB (");
	gRend.putstr(to_string(gAlloc.get_reserved_ram() / 1024 / 1024));
	gRend.putstr(" mB)");
	gRend.crlf(startOffset);
}

void srl_memory_info() {
	srl.writestr("\r\n");
	srl.writestr("Memory Info:");
	srl.writestr("\r\n");
	srl.writestr("|\\");
    srl.writestr("\r\n");
	srl.writestr("| Free RAM: ");
	srl.writestr(to_string(gAlloc.get_free_ram() / 1024));
	srl.writestr(" kB (");
	srl.writestr(to_string(gAlloc.get_free_ram() / 1024 / 1024));
	srl.writestr(" mB)");
    srl.writestr("\r\n");
	srl.writestr("|\\");
    srl.writestr("\r\n");
	srl.writestr("| Used RAM: ");
	srl.writestr(to_string(gAlloc.get_used_ram() / 1024));
	srl.writestr(" kB (");
	srl.writestr(to_string(gAlloc.get_used_ram() / 1024 / 1024));
	srl.writestr(" mB)");
    srl.writestr("\r\n");
	srl.writestr(" \\");
    srl.writestr("\r\n");
	srl.writestr("  Reserved RAM: ");
	srl.writestr(to_string(gAlloc.get_reserved_ram() / 1024));
	srl.writestr(" kB (");
	srl.writestr(to_string(gAlloc.get_reserved_ram() / 1024 / 1024));
	srl.writestr(" mB)");
	srl.writestr("\r\n");
}

void print_now(u64 xOffset = 0) {
	gRend.crlf(xOffset);
	gRend.putstr("Now is ");
	gRend.putstr(to_string((u64)gRTC.time.hour));
	gRend.putchar(':');
	gRend.putstr(to_string((u64)gRTC.time.minute));
	gRend.putchar(':');
	gRend.putstr(to_string((u64)gRTC.time.second));
	gRend.putstr(" on ");
	gRend.putstr(to_string((u64)gRTC.time.year));
	gRend.putchar('-');
	gRend.putstr(to_string((u64)gRTC.time.month));
	gRend.putchar('-');
	gRend.putstr(to_string((u64)gRTC.time.date));
	gRend.crlf(xOffset);
}

extern "C" void _start(BootInfo* bInfo) {
	// The heavy lifting is done within `kUtility.cpp`.
    kernel_init(bInfo);
	
	// Clear screen (ensure known state).
	gRend.clear();
	
	/// GPLv3 LICENSE REQUIREMENT (interactive terminal must print cpy notice).
	const char* GPLv3 = "<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>";
	// TO SERIAL
	srl.writestr("\r\n");
	srl.writestr(GPLv3);
	// TO SCREEN
	gRend.BackgroundColor = 0xffffffff;
	gRend.putstr(GPLv3, 0x00000000);
	gRend.BackgroundColor = 0x00000000;
	gRend.crlf();
	gRend.swap();
	/// END GPLv3 LICENSE REQUIREMENT.
	
	srl_memory_info();

	// Start keyboard input at draw position, not origin.
	gTextPosition = gRend.DrawPos; 
	// UPDATE SCREEN FROM TARGET BUFFER IN INFINITE LOOP.
	while (true) {
		// PRINT REAL TIME
		gRend.DrawPos = {500, 0};
		gRTC.get_date_time();
		print_now(500);
		// PRINT MEMORY INFO
		print_memory_info();
		// UPDATE ACTIVE RENDER BUFFER FROM TARGET.
		gRend.swap();
	}
	// HALT LOOP (KERNEL INACTIVE).
	while (true) {
		asm ("hlt");
	}
}
