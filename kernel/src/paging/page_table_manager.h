#ifndef LENSOR_OS_PAGE_TABLE_MANAGER
#define LENSOR_OS_PAGE_TABLE_MANAGER

#include "../integers.h"

class PageTable;
/* TODO: This is a large part of why I want to re-do the memory system.
 *   It only works with a single PML4, meaning a new manager is needed for
 *   every single page map created, which doesn't sound like much of a manager to me.
 *   I think I'm going to move the actual page map maniupulation/modification
 *   operations to the PageTable* class and create an actual PageTableManager
 *   that is able to create and manage different page tables (a separate page table
 *   will be needed by each user-land process with a specific initial setup, as
 *   the kernel must be mapped for every process.)
 */
class PageTableManager {
public:
    PageTableManager(PageTable* PML4_address);
    PageTable* PML4;

    void map_memory(void* virtualMemory, void* physicalMemory);
    void unmap_memory(void* virtualMemory);
};

extern PageTableManager gPTM;

#endif
