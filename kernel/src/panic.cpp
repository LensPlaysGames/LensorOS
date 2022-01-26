#include "panic.h"

void panic(const char* panicMessage) {
	gRend.DrawPos = {0, 400};
	gRend.drawrect({800, 200}, 0xffff0000);
	gRend.putstr("LensorOS PANIC MODE", 0x00000000);
	gRend.crlf();
	gRend.crlf();
	gRend.putstr(panicMessage);
	gRend.swap();
}
