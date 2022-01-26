#include "panic.h"

void panic(const char* panicMessage) {
	gRend.BackgroundColor = 0xffff0000;
	gRend.DrawPos = {400, 500};
	gRend.putstr("LensorOS PANIC MODE", 0x00000000);
	gRend.crlf(400/8);
	gRend.crlf(400/8);
	gRend.putstr(panicMessage);
	gRend.swap();
}
