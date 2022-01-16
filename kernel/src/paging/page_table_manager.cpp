#include "page_table_manager.h"

PageTableManager::PageTableManager(PageTable* PML4_address) {
	PML4 = PML4_address;
}

void PageTableManager::MapMemory(void* virtualMemory, void* physicalMemory) {
	PageMapIndexer indexer((uint64_t)virtualMemory);
	PageDirEntry PDE;
	
	PDE = PML4->entries[indexer.PageDirectoryPointerIndex];
	PageTable* PDP;
	if (!PDE.GetFlag(PT_Flag::Present)) {
		PDP = (PageTable*)gAlloc.RequestPage();
		memset(PDP, 0, 0x1000);
		PDE.SetAddress((uint64_t)PDP >> 12);
		PDE.SetFlag(PT_Flag::Present, true);
		PDE.SetFlag(PT_Flag::ReadWrite, true);
		PML4->entries[indexer.PageDirectoryPointerIndex] = PDE;
	}
	else {
		PDP = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
	}

	PDE = PDP->entries[indexer.PageDirectoryIndex];
	PageTable* PD;
	if (!PDE.GetFlag(PT_Flag::Present)) {
		PD = (PageTable*)gAlloc.RequestPage();
		memset(PD, 0, 0x1000);
		PDE.SetAddress((uint64_t)PD >> 12);
		PDE.SetFlag(PT_Flag::Present, true);
		PDE.SetFlag(PT_Flag::ReadWrite, true);
	    PDP->entries[indexer.PageDirectoryIndex] = PDE;
	}
	else {
		PD = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
	}

	PDE = PD->entries[indexer.PageTableIndex];
	PageTable* PT;
	if (!PDE.GetFlag(PT_Flag::Present)) {
		PT = (PageTable*)gAlloc.RequestPage();
	 	memset(PT, 0, 0x1000);
		PDE.SetAddress((uint64_t)PT >> 12);
	    PDE.SetFlag(PT_Flag::Present, true);
		PDE.SetFlag(PT_Flag::ReadWrite, true);
	   	PD->entries[indexer.PageTableIndex] = PDE;
	}
  	else {
	  	PT = (PageTable*)((uint64_t)PDE.GetAddress() << 12);
 	}

	PDE = PT->entries[indexer.PageIndex];
  	PDE.SetAddress((uint64_t)physicalMemory >> 12);
    PDE.SetFlag(PT_Flag::Present, true);
	PDE.SetFlag(PT_Flag::ReadWrite, true);
	PT->entries[indexer.PageIndex] = PDE;
}
