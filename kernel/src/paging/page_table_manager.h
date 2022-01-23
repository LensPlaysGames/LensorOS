#ifndef LENSOR_OS_PAGE_TABLE_MANAGER
#define LENSOR_OS_PAGE_TABLE_MANAGER

#include "../memory.h"
#include "paging.h"
#include "page_frame_allocator.h"

class PageTableManager {
public:
	PageTableManager(PageTable* PML4_address);
	PageTable* PML4;

	void map_memory(void* virtualMemory, void* physicalMemory);
};

extern PageTableManager gPTM;

#endif
