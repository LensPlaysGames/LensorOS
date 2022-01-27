#include "kUtility.h"

// TODO:
// - Read more of this: https://pages.cs.wisc.edu/~remzi/OSTEP/
// - FILE SYSTEM:
//  - AHCI Driver Update: DMA ATA Write implementation
//  - Another filesystem better suited for mass storage (Ext2? Proprietary?)
//  - Virtual File System that will store intermediate representation of files/folders/storage media devices
// - Save parsed PCI devices for quick lookup (device tree).
// - Write ASM interrupt wrapper (no longer rely on `__attribute__((interrupt))`)
//   - See James Molloy's tutorials for an example: http://www.jamesmolloy.co.uk/tutorial_html/
// - Test different memcpy implementations
//   - See https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
// - UART Driver (serial communication, necessary for terminals/terminal emulators).
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
	gRend.swap();
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

void print_now() {
	gRend.crlf();
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
	gRend.crlf();
	gRend.swap();
}

extern "C" void _start(BootInfo* bInfo) {
	KernelInfo info = kernel_init(bInfo);
	// Uncomment the next line to clear initial information about
	//   kernel setup (printed to screen during kernel_init).
	// gRend.clear();
	/// GPLv3 LICENSE REQUIREMENT (interactive terminal must print cpy notice).
	// TO SERIAL
	srl.writestr("\r\n");
	srl.writestr("<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>");
	// TO SCREEN
	gRend.BackgroundColor = 0xffffffff;
	gRend.putstr("<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>", 0x00000000);
	gRend.BackgroundColor = 0x00000000;
	gRend.crlf();
	gRend.swap();
	/// END GPLv3 LICENSE REQUIREMENT.
	srl_memory_info();
	print_now();
	// Start keyboard input at draw position, not origin.
	gTextPosition = gRend.DrawPos;
	// DRAW A FACE :)
	// left eye
	gRend.DrawPos = {420, 420};
	gRend.drawrect({42, 42}, 0xff00ffff);
	// left pupil
	gRend.DrawPos = {440, 440};
	gRend.drawrect({20, 20}, 0xffff0000);
	// right eye
	gRend.DrawPos = {520, 420};
	gRend.drawrect({42, 42}, 0xff00ffff);
	// right pupil
	gRend.DrawPos = {540, 440};
	gRend.drawrect({20, 20}, 0xffff0000);
	// mouth
	gRend.DrawPos = {400, 520};
	gRend.drawrect({182, 20}, 0xff00ffff);
	gRend.swap();
	// UPDATE SCREEN FROM TARGET BUFFER IN INFINITE LOOP.
	while (true) {
		// DRAW TIME ELAPSED SINCE KERNEL INITIALIZATION IN TOP RIGHT (PIT).
		gRend.DrawPos = {600, 0};
		gRend.putstr("Elapsed: ");
		gRend.putstr(to_string(get_seconds()));
		gRend.putstr("s");
		gRend.swap();
	}
	// HALT LOOP (KERNEL INACTIVE).
	while (true) {
		asm ("hlt");
	}
}
