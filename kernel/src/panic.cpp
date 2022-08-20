#include <panic.h>

#include <basic_renderer.h>
#include <cstr.h>
#include <interrupts/interrupts.h>
#include <uart.h>

Vector2<u64> PanicLocation = { PanicStartX, PanicStartY };

__attribute__((no_caller_saved_registers))
void panic(const char* panicMessage) {
    UART::out("\r\n\033[1;37;41mLensorOS PANIC\033[0m\r\n");
    UART::out("  ");
    UART::out(panicMessage);
    UART::out("\r\n");
    gRend.BackgroundColor = 0xffff0000;
    gRend.puts(PanicLocation, "LensorOS PANIC MODE");
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.puts(PanicLocation, panicMessage, 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    // Update entire bottom-right of screen starting at (PanicStartX, PanicStartY).
    gRend.swap({PanicStartX, PanicStartY}, {80000, 80000});
}

__attribute__((no_caller_saved_registers))
void panic(InterruptFrame* frame, const char* panicMessage) {
    panic(panicMessage);
    UART::out("  Instruction Address: 0x");
    UART::out(to_hexstring(frame->ip));
    UART::out("\r\n");
    UART::out("  Stack Pointer: 0x");
    UART::out(to_hexstring(frame->sp));
    UART::out("\r\n");
    gRend.puts(PanicLocation, "Instruction Address: 0x", 0x00000000);
    gRend.puts(PanicLocation, to_hexstring(frame->ip), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.puts(PanicLocation, "Stack Pointer: 0x", 0x00000000);
    gRend.puts(PanicLocation, to_hexstring(frame->sp), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    // Update entire bottom-right of screen starting at (PanicStartX, PanicStartY).
    gRend.swap({PanicStartX, PanicStartY}, {80000, 80000});
}

__attribute__((no_caller_saved_registers))
void panic(InterruptFrameError* frame, const char* panicMessage) {
    panic(panicMessage);
    UART::out("  Error Code: 0x");
    UART::out(to_hexstring(frame->error));
    UART::out("\r\n"
              "  Instruction Address: 0x"
              );
    UART::out(to_hexstring(frame->ip));
    UART::out("\r\n");
    UART::out("  Stack Pointer: 0x");
    UART::out(to_hexstring(frame->sp));
    UART::out("\r\n");
    gRend.puts(PanicLocation, "Error Code: 0x", 0x00000000);
    gRend.puts(PanicLocation, to_hexstring(frame->error), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.puts(PanicLocation, "Instruction Address: 0x", 0x00000000);
    gRend.puts(PanicLocation, to_hexstring(frame->ip), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.puts(PanicLocation, "Stack Pointer: 0x", 0x00000000);
    gRend.puts(PanicLocation, to_hexstring(frame->sp), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    // Update entire bottom-right of screen starting at (PanicStartX, PanicStartY).
    gRend.swap({PanicStartX, PanicStartY}, {80000, 80000});
}
