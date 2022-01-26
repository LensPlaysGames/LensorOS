#include "page_table_manager.h"

PageTableManager gPTM {nullptr};

PageTableManager::PageTableManager(PageTable* PML4Address) : PML4(PML4Address) {}

// Map a virtual address to a physical address.
void PageTableManager::map_memory(void* virtualMemory, void* physicalMemory) {
	PageMapIndexer indexer((u64)virtualMemory);
	PageDirEntry PDE;
	
	PDE = PML4->entries[indexer.PageDirectoryPointerIndex];
	PageTable* PDP;
	if (!PDE.get_flag(PT_Flag::Present)) {
		PDP = (PageTable*)gAlloc.request_page();
		memset(PDP, 0, 0x1000);
		PDE.set_address((u64)PDP >> 12);
		PDE.set_flag(PT_Flag::Present, true);
		PDE.set_flag(PT_Flag::ReadWrite, true);
		PML4->entries[indexer.PageDirectoryPointerIndex] = PDE;
	}
	else {
		PDP = (PageTable*)((u64)PDE.get_address() << 12);
	}

	PDE = PDP->entries[indexer.PageDirectoryIndex];
	PageTable* PD;
	if (!PDE.get_flag(PT_Flag::Present)) {
		PD = (PageTable*)gAlloc.request_page();
		memset(PD, 0, 0x1000);
		PDE.set_address((u64)PD >> 12);
		PDE.set_flag(PT_Flag::Present, true);
		PDE.set_flag(PT_Flag::ReadWrite, true);
	    PDP->entries[indexer.PageDirectoryIndex] = PDE;
	}
	else {
		PD = (PageTable*)((u64)PDE.get_address() << 12);
	}

	PDE = PD->entries[indexer.PageTableIndex];
	PageTable* PT;
	if (!PDE.get_flag(PT_Flag::Present)) {
		PT = (PageTable*)gAlloc.request_page();
	 	memset(PT, 0, 0x1000);
		PDE.set_address((u64)PT >> 12);
	    PDE.set_flag(PT_Flag::Present, true);
		PDE.set_flag(PT_Flag::ReadWrite, true);
	   	PD->entries[indexer.PageTableIndex] = PDE;
	}
  	else {
	  	PT = (PageTable*)((u64)PDE.get_address() << 12);
 	}

	PDE = PT->entries[indexer.PageIndex];
  	PDE.set_address((u64)physicalMemory >> 12);
    PDE.set_flag(PT_Flag::Present, true);
	PDE.set_flag(PT_Flag::ReadWrite, true);
	PT->entries[indexer.PageIndex] = PDE;
}
