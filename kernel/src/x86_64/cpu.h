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

#ifndef LENSOR_OS_X86_64_CPU_H
#define LENSOR_OS_X86_64_CPU_H

#include <integers.h>
#include <interrupts/interrupts.h>

#include <format>

struct CPUState {
    u64 RSP;
    u64 RBX;
    u64 RCX;
    u64 RDX;
    u64 RSI;
    u64 RDI;
    u64 RBP;
    u64 R8;
    u64 R9;
    u64 R10;
    u64 R11;
    u64 R12;
    u64 R13;
    u64 R14;
    u64 R15;
    u64 FS;
    u64 GS;
    u64 RAX;
    InterruptFrame Frame;
} __attribute__((packed));

static_assert(offsetof(CPUState, RCX) == 16);
static_assert(offsetof(CPUState, Frame) == 144);

[[nodiscard]]
bool verify(const CPUState&);

template <>
struct std::formatter<CPUState> : std::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const CPUState& cpu, FormatContext& ctx) const {
        // Construct a clean, multi-line string representation
        std::string s = std::format(
            "CPUState:\n"
            "  RAX: {:#018x}  RBX: {:#018x}  RCX: {:#018x}  RDX: {:#018x}\n"
            "  RSI: {:#018x}  RDI: {:#018x}  RBP: {:#018x}  RSP: {:#018x}\n"
            "  R8:  {:#018x}  R9:  {:#018x}  R10: {:#018x}  R11: {:#018x}\n"
            "  R12: {:#018x}  R13: {:#018x}  R14: {:#018x}  R15: {:#018x}\n"
            "  FS:  {:#018x}  GS:  {:#018x}\n"
            "  Frame::RIP: {:#018x}  Frame::CS: {:#0x}\n"
            "  Frame::RFLAGS: {:#018x}  Frame::RSP: {:#018x}  Frame::SS: {:#0x}",
            cpu.RAX,
            cpu.RBX,
            cpu.RCX,
            cpu.RDX,
            cpu.RSI,
            cpu.RDI,
            cpu.RBP,
            cpu.RSP,
            cpu.R8,
            cpu.R9,
            cpu.R10,
            cpu.R11,
            cpu.R12,
            cpu.R13,
            cpu.R14,
            cpu.R15,
            cpu.FS,
            cpu.GS,
            cpu.Frame.ip,
            cpu.Frame.cs,
            cpu.Frame.flags,
            cpu.Frame.sp,
            cpu.Frame.ss);

        // Pass the formatted string back to the base string_view formatter
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

#endif  // LENSOR_OS_X86_64_CPU_H
