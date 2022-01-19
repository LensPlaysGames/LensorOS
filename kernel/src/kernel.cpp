#include "kUtility.h"

// TODO:
// - Create basic kernel memory allocation functions (kalloc(), free()).
// - Create interface to PIT chip (timer).
// - Create cross compiler (gcc).
// - Add GPLv3 license header to top of every source file (exactly as seen in LICENSE).

extern "C" void _start(BootInfo* bInfo) {
	KernelInfo info = InitializeKernel(bInfo);
	gRend.putstr("LensorOS kernel initialized successfully");
	gRend.crlf();
	// A FACE :)
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

	// MAIN KERNEL LOOP.
	while (true) {
		ProcessMousePacket();
		gRend.swap();
	}

	// HALT LOOP (KERNEL INACTIVE).
	while (true) {
		asm ("hlt");
	}
}
