/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

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
