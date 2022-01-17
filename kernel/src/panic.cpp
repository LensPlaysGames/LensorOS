#include "panic.h"

void Panic(const char* panicMsg) {
	gRend.clear(0xffff0000);
	gRend.PixelPosition = {0, 0};
	gRend.putstr("LensorOS HAS ENTERED PANIC MODE", 0xffff0000);
	gRend.crlf();
	gRend.crlf();
	gRend.putstr(panicMsg);
}
