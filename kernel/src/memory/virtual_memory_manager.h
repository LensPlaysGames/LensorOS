#ifndef LENSOR_OS_VIRTUAL_MEMORY_MANAGER_H
#define LENSOR_OS_VIRTUAL_MEMORY_MANAGER_H

namespace Memory {
    class PageTable;

    /* Create a page map level four using the physical memory 
     *   manager, map the entire RAM address space, and 
     *   finally flush the map to use it as the active mapping.
     */
    void init_virtual();

    // TODO: Permissions handling.

    /* Map a virtual address to a physical 
     *   address in the given page map level four. 
     */
    void map(PageTable*, void* virtualAddress, void* physicalAddress);

    /* Map a virtual address to a physical address in 
     *   the currently active page map level four.
     */
    void map(void* virtualAddress, void* physicalAddress);

    /* If a mapping is marked as present within the given 
     *   page map level four, it will be marked as not present. 
     */
    void unmap(PageTable*, void* virtualAddress);

    /* If a mapping is marked as present within the given 
     *   page map level four, it will be marked as not present. 
     */
    void unmap(void* virtualAddress);

    /* Load the given address into control register three to update 
     *   the virtual to physical mapping the CPU is using currently.
     */
    void flush_page_map(PageTable* pageMapLevelFour);

    PageTable* get_active_page_map();
}

#endif /* LENSOR_OS_VIRTUAL_MEMORY_MANAGER_H */
