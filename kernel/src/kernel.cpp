#include "kUtility.h"

// TODO:
// - Keyboard Scancode Translation
//   [video resource](https://youtu.be/dtuPghvrXUo)
// - Mouse
// - Mouse Cursor
// - Add GPLv3 license header to top of every source file (exactly as seen in LICENSE)

extern "C" void _start(BootInfo* bInfo) {
	KernelInfo info = InitializeKernel(bInfo);
	gRend.putstr("LensorOS kernel initialized successfully");

	// A FACE :)
	// left eye
	gRend.PixelPosition = {420, 420};
	gRend.putrect({42, 42}, 0xff00ffff);
	// left pupil
	gRend.PixelPosition = {440, 440};
	gRend.putrect({20, 20}, 0xffff0000);
	// right eye
	gRend.PixelPosition = {520, 420};
	gRend.putrect({42, 42}, 0xff00ffff);
	// right pupil
	gRend.PixelPosition = {540, 440};
	gRend.putrect({20, 20}, 0xffff0000);
	// mouth
	gRend.PixelPosition = {400, 520};
	gRend.putrect({182, 20}, 0xff00ffff);

	gRend.PixelPosition = {0, 420};

	while (true) {
		asm ("hlt");
	}
}
