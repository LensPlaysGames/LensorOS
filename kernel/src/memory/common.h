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

#ifndef LENSOR_OS_MEMORY_COMMON_H
#define LENSOR_OS_MEMORY_COMMON_H

#include <integers.h>
#include <link_definitions.h>

/// Get the compiler to shut up about the missing `_` somehow.
/// Might just be clangd tho.
/*
size_t operator "" KiB(unsigned long long sz) {
    return sz << 10;
}
*/

#define KiB(x) ((u64)(x) << 10)
#define MiB(x) ((u64)(x) << 20)
#define GiB(x) ((u64)(x) << 30)

#define TO_KiB(x) ((u64)(x) >> 10)
#define TO_MiB(x) ((u64)(x) >> 20)
#define TO_GiB(x) ((u64)(x) >> 30)

constexpr usz PAGE_SIZE = 4096;

namespace Memory {

// Virtual Layout
#define ENSURE_USER_ADDRESS(address)           \
    static_assert(                             \
        (address & (0xffff800000000000)) == 0, \
        "User address must be lower half (Intel canonical address)");

// This is the base address where all physical memory is mapped 1:1. If
// hardware gives you a physical address and you need to access it, offset
// it by this.
constexpr uintptr_t PHYSICAL_BASE = 0xffff800000000000;

constexpr uintptr_t KERNEL_INTERRUPT_STACK_BASE = 0xffffffff10000000;

// kernel.ld
constexpr uintptr_t KERNEL_VIRTUAL_OFFSET = 0xffffffff80000000;
constexpr uintptr_t KERNEL_VIRTUAL_BASE = KERNEL_VIRTUAL_OFFSET + 0x100000;
constexpr uintptr_t KERNEL_HEAP_VIRTUAL_BASE = 0xffffffffff000000;

// Scheduler
constexpr uintptr_t USER_MEMORY_REGION_BASE = 0xf8000000;
ENSURE_USER_ADDRESS(USER_MEMORY_REGION_BASE);
constexpr uintptr_t USER_STACK_BASE = 0x0000733700000000;
ENSURE_USER_ADDRESS(USER_STACK_BASE);

}  // namespace Memory

#endif /* LENSOR_OS_MEMORY_COMMON_H */
