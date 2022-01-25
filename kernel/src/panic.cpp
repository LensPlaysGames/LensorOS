#include "panic.h"

void panic(const char* panicMessage) {
	gRend.clear(0xffff0000);
	gRend.DrawPos = {0, 0};
	gRend.putstr("LensorOS PANIC MODE", 0x00000000);
	gRend.crlf();
	gRend.crlf();
	gRend.putstr(panicMessage);
	gRend.swap();
}
