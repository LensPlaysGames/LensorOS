#include <memory/virtual_memory_manager.h>

#include <debug.h>
#include <integers.h>
#include <link_definitions.h>
#include <memory.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>

namespace Memory {
    PageTable* ActivePageMap;

    void map(PageTable* pageMapLevelFour, void* virtualAddress, void* physicalAddress, u64 mappingFlags, ShowDebug debug) {
        if (pageMapLevelFour == nullptr)
            return;
        
        PageMapIndexer indexer((u64)virtualAddress);
        PageDirectoryEntry PDE;

        bool present       = mappingFlags & static_cast<u64>(PageTableFlag::Present);
        bool write         = mappingFlags & static_cast<u64>(PageTableFlag::ReadWrite);
        bool user          = mappingFlags & static_cast<u64>(PageTableFlag::UserSuper);
        bool writeThrough  = mappingFlags & static_cast<u64>(PageTableFlag::WriteThrough);
        bool cacheDisabled = mappingFlags & static_cast<u64>(PageTableFlag::CacheDisabled);
        bool accessed      = mappingFlags & static_cast<u64>(PageTableFlag::Accessed);
        bool dirty         = mappingFlags & static_cast<u64>(PageTableFlag::Dirty);
        bool largerPages   = mappingFlags & static_cast<u64>(PageTableFlag::LargerPages);
        bool global        = mappingFlags & static_cast<u64>(PageTableFlag::Global);
        bool noExecute     = mappingFlags & static_cast<u64>(PageTableFlag::NX);

        if (debug == ShowDebug::Yes) {
            dbgmsg("Attempting to map virtual %x to physical %x in page table at %x\r\n"
                   "  Flags:\r\n"
                   "    Present:         %b\r\n"
                   "    Write:           %b\r\n"
                   "    User-accessible: %b\r\n"
                   "    Write-through:   %b\r\n"
                   "    Cache Disabled:  %b\r\n"
                   "    Accessed:        %b\r\n"
                   "    Dirty:           %b\r\n"
                   "    Larger Pages:    %b\r\n"
                   "    Global:          %b\r\n"
                   "\r\n"
                   , virtualAddress
                   , physicalAddress
                   , pageMapLevelFour
                   , present
                   , write
                   , user
                   , writeThrough
                   , cacheDisabled
                   , accessed
                   , dirty
                   , largerPages
                   , global
                   );
        }

        PDE = pageMapLevelFour->entries[indexer.page_directory_pointer()];
        PageTable* PDP;
        if (!PDE.flag(PageTableFlag::Present)) {
            PDP = (PageTable*)request_page();
            memset(PDP, 0, PAGE_SIZE);
            PDE.set_address((u64)PDP >> 12);

        }
        PDE.set_flag(PageTableFlag::Present,       present);
        PDE.set_flag(PageTableFlag::ReadWrite,     write);
        PDE.set_flag(PageTableFlag::UserSuper,     user);
        PDE.set_flag(PageTableFlag::WriteThrough,  writeThrough);
        PDE.set_flag(PageTableFlag::CacheDisabled, cacheDisabled);
        PDE.set_flag(PageTableFlag::Accessed,      accessed);
        PDE.set_flag(PageTableFlag::Dirty,         dirty);
        PDE.set_flag(PageTableFlag::LargerPages,   largerPages);
        PDE.set_flag(PageTableFlag::Global,        global);
        PDE.set_flag(PageTableFlag::Global,        noExecute);
        pageMapLevelFour->entries[indexer.page_directory_pointer()] = PDE;
        PDP = (PageTable*)((u64)PDE.address() << 12);

        PDE = PDP->entries[indexer.page_directory()];
        PageTable* PD;
        if (!PDE.flag(PageTableFlag::Present)) {
            PD = (PageTable*)request_page();
            memset(PD, 0, PAGE_SIZE);
            PDE.set_address((u64)PD >> 12);
        }
        PDE.set_flag(PageTableFlag::Present,       present);
        PDE.set_flag(PageTableFlag::ReadWrite,     write);
        PDE.set_flag(PageTableFlag::UserSuper,     user);
        PDE.set_flag(PageTableFlag::WriteThrough,  writeThrough);
        PDE.set_flag(PageTableFlag::CacheDisabled, cacheDisabled);
        PDE.set_flag(PageTableFlag::Accessed,      accessed);
        PDE.set_flag(PageTableFlag::Dirty,         dirty);
        PDE.set_flag(PageTableFlag::LargerPages,   largerPages);
        PDE.set_flag(PageTableFlag::Global,        global);
        PDE.set_flag(PageTableFlag::Global,        noExecute);
        PDP->entries[indexer.page_directory()] = PDE;
        PD = (PageTable*)((u64)PDE.address() << 12);

        PDE = PD->entries[indexer.page_table()];
        PageTable* PT;
        if (!PDE.flag(PageTableFlag::Present)) {
            PT = (PageTable*)request_page();
            memset(PT, 0, PAGE_SIZE);
            PDE.set_address((u64)PT >> 12);
        }
        PDE.set_flag(PageTableFlag::Present,       present);
        PDE.set_flag(PageTableFlag::ReadWrite,     write);
        PDE.set_flag(PageTableFlag::UserSuper,     user);
        PDE.set_flag(PageTableFlag::WriteThrough,  writeThrough);
        PDE.set_flag(PageTableFlag::CacheDisabled, cacheDisabled);
        PDE.set_flag(PageTableFlag::Accessed,      accessed);
        PDE.set_flag(PageTableFlag::Dirty,         dirty);
        PDE.set_flag(PageTableFlag::LargerPages,   largerPages);
        PDE.set_flag(PageTableFlag::Global,        global);
        PDE.set_flag(PageTableFlag::Global,        noExecute);
        PD->entries[indexer.page_table()] = PDE;
        PT = (PageTable*)((u64)PDE.address() << 12);

        PDE = PT->entries[indexer.page()];
        PDE.set_address((u64)physicalAddress >> 12);
        PDE.set_flag(PageTableFlag::Present,       present);
        PDE.set_flag(PageTableFlag::ReadWrite,     write);
        PDE.set_flag(PageTableFlag::UserSuper,     user);
        PDE.set_flag(PageTableFlag::WriteThrough,  writeThrough);
        PDE.set_flag(PageTableFlag::CacheDisabled, cacheDisabled);
        PDE.set_flag(PageTableFlag::Accessed,      accessed);
        PDE.set_flag(PageTableFlag::Dirty,         dirty);
        PDE.set_flag(PageTableFlag::LargerPages,   largerPages);
        PDE.set_flag(PageTableFlag::Global,        global);
        PDE.set_flag(PageTableFlag::Global,        noExecute);
        PT->entries[indexer.page()] = PDE;
        if (debug == ShowDebug::Yes) {
            dbgmsg_s("  \033[32mMapped\033[0m\r\n"
                     "\r\n"
                     );
        }
    }
    
    void map(void* virtualAddress, void* physicalAddress, u64 mappingFlags, ShowDebug debug) {
        map(ActivePageMap, virtualAddress, physicalAddress, mappingFlags, debug);
    }

    void unmap(PageTable* pageMapLevelFour
               , void* virtualAddress
               , ShowDebug debug)
    {
        if (debug == ShowDebug::Yes) {
            dbgmsg("Attempting to unmap virtual %p in page table at %p\r\n"
                   , virtualAddress
                   , pageMapLevelFour
                   );
        }
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
        if (debug == ShowDebug::Yes) {
            dbgmsg_s("  \033[32mUnmapped\033[0m\r\n"
                     "\r\n"
                     );
        }
    }

    void unmap(void* virtualAddress, ShowDebug d) {
        unmap(ActivePageMap, virtualAddress, d);
    }

    void flush_page_map(PageTable* pageMapLevelFour) {
        asm volatile ("mov %0, %%cr3"
                      : // No outputs
                      : "r" (pageMapLevelFour));
        ActivePageMap = pageMapLevelFour;
    }

    PageTable* clone_active_page_map() {
        // FIXME: Free already allocated pages upon failure.
        Memory::PageDirectoryEntry PDE;
        Memory::PageTable* oldPageTable = Memory::active_page_map();
        auto* newPageTable = reinterpret_cast<Memory::PageTable*>(Memory::request_page());
        if (newPageTable == nullptr) {
            dbgmsg_s("Failed to allocate memory for new process page map level four.\r\n");
            return nullptr;
        }
        memset(newPageTable, 0, PAGE_SIZE);
        for (u64 i = 0; i < 512; ++i) {
            PDE = oldPageTable->entries[i];
            if (PDE.flag(Memory::PageTableFlag::Present) == false)
                continue;

            auto* newPDP = (Memory::PageTable*)Memory::request_page();
            if (newPDP == nullptr) {
                dbgmsg_s("Failed to allocate memory for new process page directory pointer table.\r\n");
                return nullptr;
            }
            auto* oldTable = (Memory::PageTable*)((u64)PDE.address() << 12);
            for (u64 j = 0; j < 512; ++j) {
                PDE = oldTable->entries[j];
                if (PDE.flag(Memory::PageTableFlag::Present) == false)
                    continue;

                auto* newPD = (Memory::PageTable*)Memory::request_page();
                if (newPD == nullptr) {
                    dbgmsg_s("Failed to allocate memory for new process page directory table.\r\n");
                    return nullptr;
                }
                auto* oldPD = (Memory::PageTable*)((u64)PDE.address() << 12);
                for (u64 k = 0; k < 512; ++k) {
                    PDE = oldPD->entries[k];
                    if (PDE.flag(Memory::PageTableFlag::Present) == false)
                        continue;

                    auto* newPT = (Memory::PageTable*)Memory::request_page();
                    if (newPT == nullptr) {
                        dbgmsg_s("Failed to allocate memory for new process page table.\r\n");
                        return nullptr;
                    }
                    auto* oldPT = (Memory::PageTable*)((u64)PDE.address() << 12);
                    memcpy(oldPT, newPT, PAGE_SIZE);

                    PDE = oldPD->entries[k];
                    PDE.set_address((u64)newPT >> 12);
                    newPD->entries[k] = PDE;
                }
                PDE = oldTable->entries[j];
                PDE.set_address((u64)newPD >> 12);
                newPDP->entries[j] = PDE;
            }
            PDE = oldPageTable->entries[i];
            PDE.set_address((u64)newPDP >> 12);
            newPageTable->entries[i] = PDE;
        }
        return newPageTable;
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
        for (u64 t = 0; t < total_ram(); t+=PAGE_SIZE) {
            map(pageMap, (void*)t, (void*)t
                , (u64)PageTableFlag::Present
                | (u64)PageTableFlag::ReadWrite
                );
        }
        u64 kPhysicalStart = (u64)&KERNEL_PHYSICAL;
        u64 kernelBytesNeeded = 1 + ((u64)&KERNEL_END - (u64)&KERNEL_START);
        for (u64 t = kPhysicalStart; t < kPhysicalStart + kernelBytesNeeded + PAGE_SIZE; t+=PAGE_SIZE) {
            map(pageMap, (void*)(t + (u64)&KERNEL_VIRTUAL), (void*)t
                , (u64)PageTableFlag::Present
                | (u64)PageTableFlag::ReadWrite
                | (u64)PageTableFlag::Global
                );
        }
        // Make null-dereference generate exception.
        unmap(nullptr);
        // Update current page map.
        flush_page_map(pageMap);
    }

    void init_virtual() {
        init_virtual((PageTable*)Memory::request_page());
    }
}
