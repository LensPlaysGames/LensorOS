#include "kUtility.h"

// TODO:
// - AHCI Driver Implementation
// - Save parsed PCI devices for quick lookup.
// - Write ASM interrupt wrapper (no longer rely on GCC-only "__attribute__((interrupt))")
//   - See James Molloy's tutorials for an example: http://www.jamesmolloy.co.uk/tutorial_html/
// - Test different memcpy implementations
//   - See https://stackoverflow.com/questions/22387586/measuring-execution-time-of-a-function-in-c
// - Add GPLv3 license header to top of every source file (exactly as seen in LICENSE).

void print_memory_info() {
	unsigned int startX = gRend.DrawPos.x;
	gRend.putstr("Memory Info:");
	gRend.crlf(startX);
	gRend.putstr("|\\");
    gRend.crlf(startX);
	gRend.putstr("| Free RAM: ");
	gRend.putstr(to_string(gAlloc.get_free_ram() / 1024));
	gRend.putstr(" kB (");
	gRend.putstr(to_string(gAlloc.get_free_ram() / 1024 / 1024));
	gRend.putstr(" mB)");
    gRend.crlf(startX);
	gRend.putstr("|\\");
    gRend.crlf(startX);
	gRend.putstr("| Used RAM: ");
	gRend.putstr(to_string(gAlloc.get_used_ram() / 1024));
	gRend.putstr(" kB (");
	gRend.putstr(to_string(gAlloc.get_used_ram() / 1024 / 1024));
	gRend.putstr(" mB)");
    gRend.crlf(startX);
	gRend.putstr(" \\");
    gRend.crlf(startX);
	gRend.putstr("  Reserved RAM: ");
	gRend.putstr(to_string(gAlloc.get_reserved_ram() / 1024));
	gRend.putstr(" kB (");
	gRend.putstr(to_string(gAlloc.get_reserved_ram() / 1024 / 1024));
	gRend.putstr(" mB)");
	gRend.crlf(startX);
}

void print_now() {
	gRend.crlf();
	gRend.putstr("Now is ");
	gRend.putstr(to_string((uint64_t)gRTC.time.hour));
	gRend.putchar(':');
	gRend.putstr(to_string((uint64_t)gRTC.time.minute));
	gRend.putchar(':');
	gRend.putstr(to_string((uint64_t)gRTC.time.second));
	gRend.putstr(" on ");
	gRend.putstr(to_string((uint64_t)gRTC.time.year));
	gRend.putchar('-');
	gRend.putstr(to_string((uint64_t)gRTC.time.month));
	gRend.putchar('-');
	gRend.putstr(to_string((uint64_t)gRTC.time.date));
	gRend.crlf();
}

extern "C" void _start(BootInfo* bInfo) {
	KernelInfo info = kernel_init(bInfo);
	// gRend.clear();
	// GPLv3 LICENSE REQUIREMENT (interactive terminal must print cpy notice).
	gRend.BackgroundColor = 0xffffffff;
	gRend.putstr("<LensorOS>  Copyright (C) <2022>  <Rylan Lens Kellogg>", 0x00000000);
	gRend.BackgroundColor = 0x00000000;
	gRend.crlf();
	gRend.crlf();
	// END GPLv3 LICENSE REQUIREMENT.
	print_memory_info();
	gRend.crlf();
	gRend.crlf();
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

	// UPDATE SCREEN FROM TARGET BUFFER.
	while (true) {
		// DRAW TIME ELAPSED SINCE KERNEL INITIALIZATION IN TOP RIGHT.
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
