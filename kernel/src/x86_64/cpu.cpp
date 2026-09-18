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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#include <x86_64/cpu.h>
#include <x86_64/gdt.h>

#include <print>

enum class GDTEntryType {
    Invalid,
    Code,
    Data,
    MAX
};

GDTEntryType verify_gdt_offset(size_t offset) {
    if (offset >= sizeof(GDT))
        return GDTEntryType::Invalid;

    // Clear bottom 3 bits (ring selector)
    offset &= ~0b111;

    //  0x8 -> Ring0 Code
    // 0x18 -> Ring3 Code
    if (offset == 0x8
        or offset == 0x18)
        return GDTEntryType::Code;

    // 0x10 -> Ring0 Data
    // 0x20 -> Ring3 Data
    if (offset == 0x10
        or offset == 0x20)
        return GDTEntryType::Data;

    return GDTEntryType::Invalid;
}

bool verify(const CPUState& cpu) {
    // CS and DS must contain an offset into the GDT (Global Descriptor Table)
    // This offset must be valid, and point to an entry that we actually have
    // installed.
    if (verify_gdt_offset(cpu.Frame.cs) != GDTEntryType::Code) {
        std::print("[CPUVerify]: Code Segment 0x{:x} invalid\n", (uintptr_t)cpu.Frame.cs);
        return false;
    }
    if (verify_gdt_offset(cpu.Frame.ss) != GDTEntryType::Data) {
        std::print("[CPUVerify]: Code Segment 0x{:x} invalid\n", (uintptr_t)cpu.Frame.cs);
        return false;
    }

    // TODO: Verify Frame.sp is a valid stack pointer

    // TODO: Verify Frame.ip is a valid instruction pointer

    return true;
}
