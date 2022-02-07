#include "panic.h"

#include "basic_renderer.h"
#include "uart.h"
#include "interrupts/interrupts.h"

void panic(const char* panicMessage) {
    gRend.BackgroundColor = 0xffff0000;
    gRend.DrawPos = {PanicStartX, PanicStartY};
    gRend.puts("LensorOS PANIC MODE");
    srl->writestr("LensorOS PANIC\r\n");
    gRend.crlf(PanicStartX);
    gRend.crlf(PanicStartX);
    gRend.puts(panicMessage, 0x00000000);
    srl->writestr("  Message: ");
    srl->writestr(panicMessage);
    srl->writestr("\r\n");
    gRend.crlf(PanicStartX);
    // Update entire bottom-right of screen starting at (PanicStartX, PanicStartY).
    gRend.swap({PanicStartX, PanicStartY}, {80000, 80000});
}

void panic(InterruptFrame* frame, const char* panicMessage) {
    panic(panicMessage);
    gRend.puts("Instruction Address: ", 0x00000000);
    gRend.puts(to_hexstring(frame->ip), 0x00000000);
    srl->writestr("  Instruction Address: ");
    srl->writestr(to_hexstring(frame->ip));
    srl->writestr("\r\n");
    gRend.crlf(PanicStartX);
    gRend.puts("Stack Pointer: ", 0x00000000);
    gRend.puts(to_hexstring(frame->sp), 0x00000000);
    srl->writestr("  Stack Pointer: ");
    srl->writestr(to_hexstring(frame->sp));
    srl->writestr("\r\n");
    gRend.crlf(PanicStartX);
    gRend.swap({PanicStartX, PanicStartY}, {80000, 80000});
}
