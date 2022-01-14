// Custom includes
#include "BasicRenderer.h"

extern "C" void _start(Framebuffer* framebuffer, PSF1_FONT* font) {
	BasicRenderer rend(framebuffer, font);
	rend.putstr("LensorOS is becoming more powerful!");
	return;
}
