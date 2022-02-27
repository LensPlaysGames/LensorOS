#include "virtual_memory_manager.h"

#include "../integers.h"
#include "../memory.h"
#include "paging.h"
#include "physical_memory_manager.h"

namespace Memory {
    PageTable* ActivePageMap;

    void map(PageTable* pageMapLevelFour, void* virtualAddress, void* physicalAddress) {
        if (pageMapLevelFour == nullptr)
            return;
        
        PageMapIndexer indexer((u64)virtualAddress);
        PageDirectoryEntry PDE;

        PDE = pageMapLevelFour->entries[indexer.page_directory_pointer()];
        PageTable* PDP;
        if (!PDE.get_flag(PageTableFlag::Present)) {
            PDP = (PageTable*)Memory::request_page();
            memset(PDP, 0, 0x1000);
            PDE.set_address((u64)PDP >> 12);
            PDE.set_flag(PageTableFlag::Present, true);
            PDE.set_flag(PageTableFlag::ReadWrite, true);
            PDE.set_flag(PageTableFlag::UserSuper, true);
            pageMapLevelFour->entries[indexer.page_directory_pointer()] = PDE;
        }
        else PDP = (PageTable*)((u64)PDE.get_address() << 12);

        PDE = PDP->entries[indexer.page_directory()];
        PageTable* PD;
        if (!PDE.get_flag(PageTableFlag::Present)) {
            PD = (PageTable*)Memory::request_page();
            memset(PD, 0, 0x1000);
            PDE.set_address((u64)PD >> 12);
            PDE.set_flag(PageTableFlag::Present, true);
            PDE.set_flag(PageTableFlag::ReadWrite, true);
            PDE.set_flag(PageTableFlag::UserSuper, true);
            PDP->entries[indexer.page_directory()] = PDE;
        }
        else PD = (PageTable*)((u64)PDE.get_address() << 12);

        PDE = PD->entries[indexer.page_table()];
        PageTable* PT;
        if (!PDE.get_flag(PageTableFlag::Present)) {
            PT = (PageTable*)Memory::request_page();
            memset(PT, 0, 0x1000);
            PDE.set_address((u64)PT >> 12);
            PDE.set_flag(PageTableFlag::Present, true);
            PDE.set_flag(PageTableFlag::ReadWrite, true);
            PDE.set_flag(PageTableFlag::UserSuper, true);
            PD->entries[indexer.page_table()] = PDE;
        }
        else PT = (PageTable*)((u64)PDE.get_address() << 12);

        PDE = PT->entries[indexer.page()];
        PDE.set_address((u64)physicalAddress >> 12);
        PDE.set_flag(PageTableFlag::Present, true);
        PDE.set_flag(PageTableFlag::ReadWrite, true);
    
        // FIXME FIXME FIXME
        // NOT ALL PAGES SHOULD BE ACCESSIBLE TO USER MODE!!!
        PDE.set_flag(PageTableFlag::UserSuper, true);
    
        PT->entries[indexer.page()] = PDE;
    }
    
    void map(void* virtualAddress, void* physicalAddress) {
        map(ActivePageMap, virtualAddress, physicalAddress);
    }

    void unmap(PageTable* pageMapLevelFour, void* virtualAddress) {
        PageMapIndexer indexer((u64)virtualAddress);
        PageDirectoryEntry PDE;
        PDE = pageMapLevelFour->entries[indexer.page_directory_pointer()];
        PageTable* PDP = (PageTable*)((u64)PDE.get_address() << 12);
        PDE = PDP->entries[indexer.page_directory()];
        PageTable* PD = (PageTable*)((u64)PDE.get_address() << 12);
        PDE = PD->entries[indexer.page_table()];
        PageTable* PT = (PageTable*)((u64)PDE.get_address() << 12);
        PDE = PT->entries[indexer.page()];
        PDE.set_flag(PageTableFlag::Present, false);
        PT->entries[indexer.page()] = PDE;
    }

    void unmap(void* virtualAddress) {
        unmap(ActivePageMap, virtualAddress);
    }

    void flush_page_map(PageTable* pageMapLevelFour) {
        asm volatile ("mov %0, %%cr3"
                      : // No outputs
                      : "r" (pageMapLevelFour));
        ActivePageMap = pageMapLevelFour;
    }

    PageTable* get_active_page_map() {
        if (ActivePageMap)
            return ActivePageMap;

        PageTable* ret { 0 };
        asm volatile ("mov %%cr3, %%rax\n\t"
                      "mov %%rax, %0"
                      : "=m"(ret)
                      : // No inputs
                      : "rax");
        return ret;
    }

    void init_virtual() {
        PageTable* initialPageMap = (PageTable*)Memory::request_page();
        /* Map all physical RAM addresses to virtual 
         *   addresses 1:1, store them in the PML4.
         * This means that virtual addresses will be
         *   equal to physical memory addresses.
         */
        for (u64 t = 0; t < Memory::get_total_ram(); t+=0x1000)
            map(initialPageMap, (void*)t, (void*)t);

        flush_page_map(initialPageMap);
    }
}