#include <memory/virtual_memory_manager.h>

#include <integers.h>
#include <link_definitions.h>
#include <memory.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>

namespace Memory {
    PageTable* ActivePageMap;

    void map(PageTable* pageMapLevelFour, void* virtualAddress, void* physicalAddress) {
        if (pageMapLevelFour == nullptr)
            return;
        
        PageMapIndexer indexer((u64)virtualAddress);
        PageDirectoryEntry PDE;

        PDE = pageMapLevelFour->entries[indexer.page_directory_pointer()];
        PageTable* PDP;
        if (!PDE.flag(PageTableFlag::Present)) {
            PDP = (PageTable*)request_page();
            memset(PDP, 0, PAGE_SIZE);
            PDE.set_address((u64)PDP >> 12);
            PDE.set_flag(PageTableFlag::Present, true);
            PDE.set_flag(PageTableFlag::ReadWrite, true);
            PDE.set_flag(PageTableFlag::UserSuper, true);
            pageMapLevelFour->entries[indexer.page_directory_pointer()] = PDE;
        }
        else PDP = (PageTable*)((u64)PDE.address() << 12);

        PDE = PDP->entries[indexer.page_directory()];
        PageTable* PD;
        if (!PDE.flag(PageTableFlag::Present)) {
            PD = (PageTable*)request_page();
            memset(PD, 0, PAGE_SIZE);
            PDE.set_address((u64)PD >> 12);
            PDE.set_flag(PageTableFlag::Present, true);
            PDE.set_flag(PageTableFlag::ReadWrite, true);
            PDE.set_flag(PageTableFlag::UserSuper, true);
            PDP->entries[indexer.page_directory()] = PDE;
        }
        else PD = (PageTable*)((u64)PDE.address() << 12);

        PDE = PD->entries[indexer.page_table()];
        PageTable* PT;
        if (!PDE.flag(PageTableFlag::Present)) {
            PT = (PageTable*)request_page();
            memset(PT, 0, PAGE_SIZE);
            PDE.set_address((u64)PT >> 12);
            PDE.set_flag(PageTableFlag::Present, true);
            PDE.set_flag(PageTableFlag::ReadWrite, true);
            PDE.set_flag(PageTableFlag::UserSuper, true);
            PD->entries[indexer.page_table()] = PDE;
        }
        else PT = (PageTable*)((u64)PDE.address() << 12);

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
        PageTable* PDP = (PageTable*)((u64)PDE.address() << 12);
        PDE = PDP->entries[indexer.page_directory()];
        PageTable* PD = (PageTable*)((u64)PDE.address() << 12);
        PDE = PD->entries[indexer.page_table()];
        PageTable* PT = (PageTable*)((u64)PDE.address() << 12);
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

    PageTable* active_page_map() {
        if (!ActivePageMap) {
            asm volatile ("mov %%cr3, %%rax\n\t"
                          "mov %%rax, %0"
                          : "=m"(ActivePageMap)
                          : // No inputs
                          : "rax");
        }
        return ActivePageMap;
    }

    void init_virtual(PageTable* pageMap) {
        /* Map all physical RAM addresses to virtual 
         *   addresses 1:1, store them in the PML4.
         * This means that virtual memory addresses will be
         *   equal to physical memory addresses within the kernel.
         */

        for (u64 t = 0; t < total_ram(); t+=PAGE_SIZE)
            map(pageMap, (void*)t, (void*)t);

        u64 kPhysicalStart = (u64)&KERNEL_PHYSICAL;
        u64 kernelBytesNeeded = 1 + ((u64)&KERNEL_END - (u64)&KERNEL_START);
        for (u64 t = kPhysicalStart; t < kPhysicalStart + kernelBytesNeeded + PAGE_SIZE; t+=PAGE_SIZE)
            map(pageMap, (void*)(t + (u64)&KERNEL_VIRTUAL), (void*)t);

        // Make null-dereference generate exception.
        unmap(nullptr);

        flush_page_map(pageMap);
    }

    void init_virtual() {
        init_virtual((PageTable*)Memory::request_page());
    }
}
