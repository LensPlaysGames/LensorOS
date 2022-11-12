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

#include <cpuid.h>

void cpuid(u32 code, u32& a, u32& b, u32& c, u32& d) {
    /* Inline Assembly:
     * |- Desc:
     * |  `- `cpuid`  -- Returns information about the CPU based on code in EAX.
     * |- Outputs:
     * |  |- "=a" -- RAX; Write-only.
     * |  |- "=b" -- RBX; Write-only.
     * |  |- "=c" -- RCX; Write-only.
     * |  `- "=d" -- RDX; Write-only.
     * `- Inputs:
     *    `- "a"  -- RAX; the code that is input to CPUID.
     */
    asm volatile ("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d): "a"(code));
}

// Strings are returned in registers 'B', 'D', and 'C'
// This structure allows it's address to be treated
//   as a valid and human-readable c-string.
struct CPUIDString {
    u32 B { 0 };
    u32 D { 0 };
    u32 C { 0 };
    u8 NULL_TERMINATOR { 0 };
    u32 A { 0 };
};

CPUIDString cpuidStringBuffer;
char* cpuid_string(u32 code) {
    cpuidStringBuffer = CPUIDString();
    cpuid(code, cpuidStringBuffer.A, cpuidStringBuffer.B, cpuidStringBuffer.C, cpuidStringBuffer.D);
    return (char*)&cpuidStringBuffer;
}
