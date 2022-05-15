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

    /* If a mapping is marked as present within the given 
     *   page map level four, it will be marked as not present. 
     */
    void unmap(PageTable*, void* virtualAddress
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

    /* Return the base address of an exact copy of the currently active page map.
     * NOTE: Does not map itself, or unmap physical identity mapping.
     */
    PageTable* clone_active_page_map();

    /// Return the base address of the currently active page map.
    PageTable* active_page_map();
}

#endif /* LENSOR_OS_VIRTUAL_MEMORY_MANAGER_H */
