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


#include <basic_renderer.h>
#include <cstr.h>
#include <format>
#include <interrupts/interrupts.h>
#include <panic.h>

Vector2<u64> PanicLocation = { PanicStartX, PanicStartY };

__attribute__((no_caller_saved_registers))
void panic(const char* message) {
    std::print("\n\033[1;37;41mLensorOS PANIC\033[0m\n  {}\n", message);
    gRend.BackgroundColor = 0xffff0000;
    gRend.puts(PanicLocation, "LensorOS PANIC MODE");
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.puts(PanicLocation, message, 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    // Update entire bottom-right of screen starting at (PanicStartX, PanicStartY).
    gRend.swap({PanicStartX, PanicStartY}, {80000, 80000});
}

__attribute__((no_caller_saved_registers))
void panic(InterruptFrame* frame, const char* panicMessage) {
    panic(panicMessage);
    std::print("  Instruction Address: {:#016x}\n"
               "  Stack Pointer: {:#016x}\n"
               , u64(frame->ip)
               , u64(frame->sp));
    gRend.puts(PanicLocation, std::format("Instruction Address: {:#016x}", u64(frame->ip)), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.puts(PanicLocation, std::format("Stack Pointer: {:#016x}", u64(frame->sp)), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    // Update entire bottom-right of screen starting at (PanicStartX, PanicStartY).
    gRend.swap({PanicStartX, PanicStartY}, {80000, 80000});
}

__attribute__((no_caller_saved_registers))
void panic(InterruptFrameError* frame, const char* panicMessage) {
    panic(panicMessage);
    std::print("  Error Code: {:#016x}\n"
               "  Instruction Address: {:#016x}\n"
               "  Stack Pointer: {:#016x}\n"
               , u64(frame->error)
               , u64(frame->ip)
               , u64(frame->sp));
    gRend.puts(PanicLocation, std::format("Error Code: {:#016x}", u64(frame->error)), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.puts(PanicLocation, std::format("Instruction Address: {:#016x}", u64(frame->ip)), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    gRend.puts(PanicLocation, std::format("Stack Pointer: {:#016x}", u64(frame->sp)), 0x00000000);
    gRend.crlf(PanicLocation, PanicStartX);
    // Update entire bottom-right of screen starting at (PanicStartX, PanicStartY).
    gRend.swap({PanicStartX, PanicStartY}, {80000, 80000});
}
