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

#include <memory/api.h>
#include <memory/types.h>

namespace Memory {

memory_space_handle startup_handle{0};

void map_region(
    memory_space_handle handle,
    VirtualAddress virtual_address,
    PhysicalAddress physical_address,
    uintptr_t byte_count,
    Flags flags) {
    // TODO: now
    // used by early UART setup.
    // map a virtual region in the page table pointed to by handle.

    // TODO: later
    // map large pages

    // TODO: later
    // split large pages
}

void unmap_region(
    memory_space_handle handle,
    VirtualAddress address,
    uintptr_t byte_count) {
    // TODO: now
    // unmap a virtual region in the page table pointed to by handle.

    // TODO: later
    // map large pages

    // TODO: later
    // split large pages
}

memory_space_handle active_memory_space() {
    // inline assembly to fetch kernel memory space pointer.
    // used by early UART setup.
    // used by physical memory manager.
    Memory::memory_space_handle memory_handle{};
    asm volatile(
        "mrs %0, ttbr1_el1\n\t"
        : "=r"(memory_handle)
        : /** (!) No Inputs (!) **/
        : "memory");
    return memory_handle;
}

memory_space_handle copy_memory_space(memory_space_handle handle) {
    // TODO: later
    // not necessary for function.
    return 0;
}

void free_memory_space(memory_space_handle handle) {
    // TODO: later
    // not necessary for function.
}

void print_memory_space(memory_space_handle handle) {
    // TODO: later
    // not necessary for function.
}

void invalidate_tlb() {
    // inline assembly to invalidate entire virtual address translation cache,
    // and re-build it from memory.
    // used by physical memory manager.
    asm volatile(
        "tlbi vmalle1\n\t"  // Invalidate all stage 1 TLB entries for EL1
        "dsb sy\n\t"        // Ensure TLB invalidation completes before continuing
        "isb"               // Flush pipeline to use the updated translation context
        :                   /* No Outputs */
        :                   /* No Inputs */
        : "memory"          // Tell compiler memory has changed drastically
    );
}

memory_space_handle get_user_memory_space() {
    // inline assembly to fetch ttbr0_el1
    Memory::memory_space_handle memory_handle{};
    asm volatile(
        "mrs %0, ttbr0_el1"
        : "=r"(memory_handle)
        : /** (!) No Inputs (!) **/
        : "memory");
    return memory_handle;
}
void set_user_memory_space(memory_space_handle handle) {
    // inline assembly to set ttbr0_el1
    asm volatile(
        "msr ttbr0_el1, %0\n\t"
        "isb"
        : /** (!) No Outputs (!) **/
        : "r"(handle)
        : "memory");
}

}  // namespace Memory
