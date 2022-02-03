#include "panic.h"

void panic(const char* panicMessage) {
    gRend.BackgroundColor = 0xffff0000;
    gRend.DrawPos = {400, 500};
    gRend.puts("LensorOS PANIC MODE");
    gRend.crlf(400);
    gRend.crlf(400);
    gRend.puts(panicMessage, 0x00000000);
    gRend.swap();
}
