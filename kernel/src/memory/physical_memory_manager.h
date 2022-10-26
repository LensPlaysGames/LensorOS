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

#ifndef LENSOR_OS_PHYSICAL_MEMORY_MANAGER_H
#define LENSOR_OS_PHYSICAL_MEMORY_MANAGER_H

#include <bitmap.h>
#include <efi_memory.h>
#include <linked_list.h>

namespace Memory {
    void init_physical(EFI_MEMORY_DESCRIPTOR* map, u64 size, u64 entrySize);

    /* Returns the total amount of RAM in bytes. */
    u64 total_ram();
    /* Returns the amount of free RAM in bytes. */
    u64 free_ram();
    /* Returns the amount of used RAM in bytes. */
    u64 used_ram();

    /* Return the physical address of the base of a free
     *   page in memory, while locking it at the same time.
     */
    void* request_page();
    /* Return the physical address of a contiguous region of physical
     *   memory that is guaranteed to have the next `numberOfPages`
     *   pages free, while locking all of them before returning.
     */
    void* request_pages(u64 numberOfPages);

    void lock_page(void* address);
    void lock_pages(void* address, u64 numberOfPages);

    void free_page(void* address);
    void free_pages(void* address, u64 numberOfPages);

    void print_debug();
    void print_debug_kib();
    void print_debug_mib();
}

#endif /* LENSOR_OS_PHYSICAL_MEMORY_MANAGER_H */
