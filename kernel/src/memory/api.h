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

#ifndef LENSOR_OS_HIGH_LEVEL_MEMORY_API_H
#define LENSOR_OS_HIGH_LEVEL_MEMORY_API_H

#include <memory/types.h>

namespace Memory {

/** TODO:
 ** Ensure the following Memory API is ready to use, including requesting
 ** new physical memory frames and mapping those physical frames to
 ** arbitrary virtual addresses.
 **/
void initialize();

/** NOTE: You should probably use a higher level helper.
 **
 ** Map the specified virtual address to the specified physical
 ** address. Do this in the memory space of the specified handle.
 **
 ** This action redirects all memory accesses from the virtual address
 ** range to the physical address range. The virtual address range starts
 ** at the virtual address. It ends at the sum of the virtual address and
 ** the byte count. Memory accesses include read operations and write
 ** operations.
 **/
void map_region(
    memory_space_handle handle,
    VirtualAddress virtual_address,
    PhysicalAddress physical_address,
    uintptr_t byte_count,
    Flags flags);

/** Map a region of memory in the specified space for kernel (read and
 ** write) access.
 **/
inline void map_region_kernel(
    memory_space_handle handle,
    VirtualAddress virtual_address,
    PhysicalAddress physical_address,
    uintptr_t byte_count) {
    map_region(
        handle,
        virtual_address,
        physical_address,
        byte_count,
        Memory::Flag::Readable | Memory::Flag::Writable);
}

/** Map a region of memory in the specified space for user (read and
 ** write) access.
 **/
inline void map_region_user(
    memory_space_handle handle,
    VirtualAddress virtual_address,
    PhysicalAddress physical_address,
    uintptr_t byte_count) {
    map_region(
        handle,
        virtual_address,
        physical_address,
        byte_count,
        Memory::Flag::Readable | Memory::Flag::Writable | Memory::Flag::User);
}

/** Map a region of memory in the specified space for kernel (read and
 ** write) access, disabling CPU caching on both reads and writes to this
 ** memory.
 **/
inline void map_region_mmio(
    memory_space_handle handle,
    VirtualAddress virtual_address,
    PhysicalAddress physical_address,
    uintptr_t byte_count) {
    map_region(
        handle,
        virtual_address,
        physical_address,
        byte_count,
        Memory::Flag::Readable
            | Memory::Flag::Writable
            | Memory::Flag::DisableReadCache
            | Memory::Flag::DisableWriteCache);
}

/** Get a PhysicalKernelAddress (higher half 1:1 physical mapping)
 ** representing a contiguous amount of frames of free physical memory at
 ** least large enough to hold the requested amount of bytes.
 **
 ** This is the primitive, low-level memory allocation mechanism on which
 ** "everything else" is based.
 **
 ** @return beginning address of physically contiguous memory frames now
 ** marked as allocated.
 **/
PhysicalKernelAddress request_memory(size_t byte_amount);

}  // namespace Memory

#endif  // LENSOR_OS_HIGH_LEVEL_MEMORY_API_H
