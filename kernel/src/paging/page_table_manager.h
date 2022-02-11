#ifndef LENSOR_OS_PAGE_TABLE_MANAGER
#define LENSOR_OS_PAGE_TABLE_MANAGER

#include "../integers.h"

class PageTable;

class PageTableManager {
public:
    PageTableManager(PageTable* PML4_address);
    PageTable* PML4;

    void map_memory(void* virtualMemory, void* physicalMemory);
};

extern PageTableManager gPTM;

#endif
