#include "panic.h"

void panic(const char* panicMessage) {
	gRend.BackgroundColor = 0xffff0000;
	gRend.DrawPos = {400, 500};
	gRend.putstr("LensorOS PANIC MODE");
	gRend.newl();
	gRend.newl();
	gRend.putstr(panicMessage, 0x00000000);
	gRend.swap();
}
