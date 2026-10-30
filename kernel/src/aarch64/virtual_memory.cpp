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

#include <memory.h>
#include <memory/api.h>
#include <memory/types.h>

constexpr uint64_t entry_present = 1;

constexpr uint64_t entry_page = 0b11;
constexpr uint64_t entry_next_table = 0b11;
constexpr uint64_t entry_large_page = 0b01;
constexpr uint64_t entry_address_mask = ~uint64_t(0x1ff);

namespace Memory {

memory_space_handle startup_handle{0};

void map_region(
    memory_space_handle handle,
    VirtualAddress virtual_address,
    PhysicalAddress physical_address,
    uintptr_t byte_count,
    Flags flags) {
    // map a virtual region in the page table pointed to by handle.

    // TODO: flags combination -> MAIR index
    uint mair_index = 0;

    uintptr_t offset = 0;
    while (offset < byte_count) {
        const auto va = virtual_address.address + offset;
        const auto pa = physical_address.address + offset;

        uintptr_t depth0_index = (va >> 39) & 0x1ff;
        uint64_t* depth0_table = (uint64_t*)handle;
        uint64_t* depth0_entry_ptr = &depth0_table[depth0_index];

        // if depth0_entry is not valid/present, we should allocate a table
        // to go there, and install it there.
        if (not(*depth0_entry_ptr & entry_present)) {
            auto new_table = Memory::request_memory(PAGE_SIZE);
            memset((void*)new_table.address, 0, PAGE_SIZE);
            *depth0_entry_ptr = PhysicalAddress(new_table).address
                                | entry_next_table;
        }

        uintptr_t depth1_index = (va >> 30) & 0x1ff;
        uint64_t* depth1_table = (uint64_t*)PhysicalKernelAddress(
                                     PhysicalAddress(*depth0_entry_ptr & entry_address_mask))
                                     .address;
        uint64_t* depth1_entry_ptr = &depth1_table[depth1_index];
        if (not(*depth1_entry_ptr & entry_present)) {
            auto new_table = Memory::request_memory(PAGE_SIZE);
            memset((void*)new_table.address, 0, PAGE_SIZE);
            *depth1_entry_ptr = PhysicalAddress(new_table).address
                                | entry_next_table;
        }

        uintptr_t depth2_index = (va >> 21) & 0x1ff;
        uint64_t* depth2_table = (uint64_t*)PhysicalKernelAddress(
                                     PhysicalAddress(*depth1_entry_ptr & entry_address_mask))
                                     .address;
        uint64_t* depth2_entry_ptr = &depth2_table[depth2_index];

        bool can_map_2mib = ((byte_count - offset) >= PAGE_SIZE_LARGE)
                            and ((va & (PAGE_SIZE_LARGE - 1)) == 0)
                            and ((pa & (PAGE_SIZE_LARGE - 1)) == 0);

        if (can_map_2mib) {
            // Write a Depth 2 Block Descriptor (0b01) directly and advance by 2MB
            *depth2_entry_ptr = (pa & entry_address_mask)
                                | ((mair_index << 2) & 0b111)
                                | entry_large_page;
            offset += PAGE_SIZE_LARGE;
            continue;
        }

        // --- Handle Page Splitting ---
        if (((*depth2_entry_ptr & entry_large_page) == entry_large_page)) {
            uint64_t existing_large_entry = *depth2_entry_ptr;
            uintptr_t large_page_pa = existing_large_entry & entry_address_mask;
            uint64_t large_page_flags = existing_large_entry
                                        & ~entry_address_mask
                                        & ~uint64_t(0b11);

            // Allocate a new table for Depth 3
            auto new_table = Memory::request_memory(PAGE_SIZE);
            uint64_t* depth3_sub_table = (uint64_t*)new_table.address;

            // Populate the new sub-table with 512 entries mapping out the original 2MB block cleanly
            for (int i = 0; i < 512; ++i) {
                uintptr_t individual_page_pa = large_page_pa + (i * PAGE_SIZE);
                depth3_sub_table[i] = (individual_page_pa & entry_address_mask)
                                      | large_page_flags
                                      | entry_page;
            }

            // Update Depth 2 entry to target new split table
            *depth2_entry_ptr = (PhysicalAddress(new_table).address & entry_address_mask)
                                | entry_next_table;
        }
        else if (not(*depth2_entry_ptr & entry_present)) {
            auto new_table = Memory::request_memory(PAGE_SIZE);
            memset((void*)new_table.address, 0, PAGE_SIZE);
            *depth2_entry_ptr = PhysicalAddress(new_table).address
                                | entry_next_table;
        }

        uintptr_t depth3_index = (va >> 12) & 0x1ff;
        uint64_t* depth3_table = (uint64_t*)PhysicalKernelAddress(
                                     PhysicalAddress(*depth2_entry_ptr & entry_address_mask))
                                     .address;
        uint64_t* depth3_entry_ptr = &depth3_table[depth3_index];
        *depth3_entry_ptr = (pa & entry_address_mask)
                            // TODO: shareability and stuff (other bits of entry)
                            | ((mair_index << 2) & 0b111)
                            | entry_page;
        offset += PAGE_SIZE;
    }
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
