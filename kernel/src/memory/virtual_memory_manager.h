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

#ifndef LENSOR_OS_VIRTUAL_MEMORY_MANAGER_H
#define LENSOR_OS_VIRTUAL_MEMORY_MANAGER_H

#include <integers.h>
#include <memory/paging.h>

namespace Memory {
    /* Map the entire physical address space, virtual kernel space, and
     *   finally flush the map to use it as the active mapping.
     */
    void init_virtual(PageTable*);
    void init_virtual();

    enum class ShowDebug {
        Yes = 0,
        No  = 1,
    };

    /* Map a virtual address to a physical
     *   address in the given page map level four.
     */
    void map(PageTable*
             , void* virtualAddress
             , void* physicalAddress
             , u64 mappingFlags
             , ShowDebug d = ShowDebug::No
             );

    /* Map a virtual address to a physical address in
     *   the currently active page map level four.
     */
    void map(void* virtualAddress
             , void* physicalAddress
             , u64 mappingFlags
             , ShowDebug d = ShowDebug::No
             );

    /* If a mapping is within the range beginning at the given virtual
     * address and spanning the given length in pages is marked as
     * present within the given page map level four, it will be marked
     * with the flags found in `mappingFlags`, and mapped to the
     * corresponding contiguous physical address.
    */
    void map_pages(PageTable* pageTable
                   , void* virtualAddress
                   , void* physicalAddress
                   , u64 mappingFlags
                   , usz pageCount
                   , ShowDebug d
                   );

    /* If a mapping is marked as present within the given
     *   page map level four, it will be marked as not present.
     */
    void unmap(PageTable*, void* virtualAddress
               , ShowDebug d = ShowDebug::No
               );

    /* If a mapping is within the range beginning at the given virtual
     * address and spanning the given length in pages is marked as
     * present within the given page map level four, it will be marked
     * as not present.
    */
    void unmap_pages(PageTable*, void* virtualAddress
                     , usz pageCount
                     , ShowDebug d = ShowDebug::No
                     );

    /* If a mapping is marked as present within the given
     *   page map level four, it will be marked as not present.
     */
    void unmap(void* virtualAddress
               , ShowDebug d = ShowDebug::No
               );

    /* Load the given address into control register three to update
     *   the virtual to physical mapping the CPU is using currently.
     */
    void flush_page_map(PageTable* pageMapLevelFour);

    /* Return the base address of an exact copy of the given page map.
     * NOTE: Does not map itself, or unmap physical identity mapping.
     */
    Memory::PageTable* clone_page_map(Memory::PageTable* oldPageTable);

    /* Free the physical memory used to describe the given page table.
     * DO NOT try to free the currently active page map!
     */
    void free_page_map(PageTable* pageTable);

    /* Return the base address of an exact copy of the currently active page map.
     * NOTE: Does not map itself, or unmap physical identity mapping.
     */
    PageTable* clone_active_page_map();

    /// Return the base address of the currently active page map.
    PageTable* active_page_map();

    /// Print present ranges of addresses that share all flags.
    /// If filter is given, only show ranges with the given flag(s)
    /// enabled.
    void print_page_map(Memory::PageTable*, Memory::PageTableFlag filter = Memory::PageTableFlag::Present);

    /// Print the flags of a page directory entry. If no flags are set,
    /// nothing will be printed.
    void print_pde_flags(Memory::PageDirectoryEntry PDE);
}

#endif /* LENSOR_OS_VIRTUAL_MEMORY_MANAGER_H */
