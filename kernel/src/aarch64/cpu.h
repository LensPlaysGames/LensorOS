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

#ifndef LENSOR_OS_AARCH64_CPU_H
#define LENSOR_OS_AARCH64_CPU_H

#include <integers.h>

#include <format>

constexpr inline uint64_t rdtsc() {
    uint64_t out;
    asm volatile("mrs %0, cntvct_el0" : "=r"(out));
    return out;
}

struct CPUState {
    u64 X0;
    u64 X1;
    u64 X2;
    u64 X3;
    u64 X4;
    u64 X5;
    u64 X6;
    u64 X7;
    u64 X8;
    u64 X9;
    u64 X10;
    u64 X11;
    u64 X12;
    u64 X13;
    u64 X14;
    u64 X15;
    u64 X16;
    u64 X17;
    u64 X18;
    u64 X19;
    u64 X20;
    u64 X21;
    u64 X22;
    u64 X23;
    u64 X24;
    u64 X25;
    u64 X26;
    u64 X27;
    u64 X28;
    u64 X29;
    u64 X30;
    u64 X31;
} __attribute__((packed));

inline u64& syscall_return_register(CPUState& cpu) {
    return cpu.X0;
}

[[nodiscard]]
bool verify(const CPUState&);

template <>
struct std::formatter<CPUState> : std::formatter<std::string_view> {
    template <typename FormatContext>
    auto format(const CPUState& cpu, FormatContext& ctx) const {
        // Construct a clean, multi-line string representation
        std::string s = std::format(
            "CPUState:\n"
            "  X0:  {:#018x}  X1:  {:#018x}  X2:  {:#018x}  X3:  {:#018x}\n"
            "  X4:  {:#018x}  X5:  {:#018x}  X6:  {:#018x}  X7:  {:#018x}\n"
            "  X8:  {:#018x}  X9:  {:#018x}  X10: {:#018x}  X11: {:#018x}\n"
            "  X12: {:#018x}  X13: {:#018x}  X14: {:#018x}  X15: {:#018x}\n"
            "  X16: {:#018x}  X17: {:#018x}  X18: {:#018x}  X19: {:#018x}\n"
            "  X20: {:#018x}  X21: {:#018x}  X22: {:#018x}  X23: {:#018x}\n"
            "  X24: {:#018x}  X25: {:#018x}  X26: {:#018x}  X27: {:#018x}\n"
            "  X28: {:#018x}  X29: {:#018x}  X30: {:#018x}  X31: {:#018x}\n",
            cpu.X0,
            cpu.X1,
            cpu.X2,
            cpu.X3,
            cpu.X4,
            cpu.X5,
            cpu.X6,
            cpu.X7,
            cpu.X8,
            cpu.X9,
            cpu.X10,
            cpu.X11,
            cpu.X12,
            cpu.X13,
            cpu.X14,
            cpu.X15,
            cpu.X16,
            cpu.X17,
            cpu.X18,
            cpu.X19,
            cpu.X20,
            cpu.X21,
            cpu.X22,
            cpu.X23,
            cpu.X24,
            cpu.X25,
            cpu.X26,
            cpu.X27,
            cpu.X28,
            cpu.X29,
            cpu.X30,
            cpu.X31);

        // Pass the formatted string back to the base string_view formatter
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

#endif  // LENSOR_OS_AARCH64_CPU_H
