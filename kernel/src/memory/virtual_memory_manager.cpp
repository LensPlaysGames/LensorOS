/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#include <debug.h>
#include <integers.h>
#include <link_definitions.h>
#include <memory.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>

#include <print>

namespace Memory {
PageTable* ActivePageMap;

void map(PageTable* pageMapLevelFour, void* virtualAddress, void* physicalAddress, u64 mappingFlags, ShowDebug debug) {
    if (pageMapLevelFour == nullptr)
        return;

    PageMapIndexer indexer((u64)virtualAddress);
    PageDirectoryEntry PDE;

    bool present = mappingFlags & static_cast<u64>(PageTableFlag::Present);
    bool write = mappingFlags & static_cast<u64>(PageTableFlag::ReadWrite);
    bool user = mappingFlags & static_cast<u64>(PageTableFlag::UserSuper);
    bool writeThrough = mappingFlags & static_cast<u64>(PageTableFlag::WriteThrough);
    bool cacheDisabled = mappingFlags & static_cast<u64>(PageTableFlag::CacheDisabled);
    bool global = mappingFlags & static_cast<u64>(PageTableFlag::Global);
    // bool noExecute     = mappingFlags & static_cast<u64>(PageTableFlag::NX);

    if (debug == ShowDebug::Yes) {
        std::print(
            "Attempting to map virtual {} to physical {} in page table at {}\n"
            "  Flags:\n"
            "    Present:         {}\n"
            "    Write:           {}\n"
            "    User-accessible: {}\n"
            "    Write-through:   {}\n"
            "    Cache Disabled:  {}\n"
            "    Global:          {}\n"
            "\n",
            virtualAddress,
            physicalAddress,
            (void*)pageMapLevelFour,
            present,
            write,
            user,
            writeThrough,
            cacheDisabled,
            global);
    }

    PDE = pageMapLevelFour->entries[indexer.page_directory_pointer()];
    PageTable* PDP;
    if (!PDE.flag(PageTableFlag::Present)) {
        PDP = (PageTable*)request_page();
        memset(PDP, 0, PAGE_SIZE);
        PDE.set_address((u64)PDP);
    }
    PDE.or_flag_if(PageTableFlag::Present, present);
    PDE.or_flag_if(PageTableFlag::ReadWrite, write);
    PDE.or_flag_if(PageTableFlag::UserSuper, user);
    // PDE.or_flag_if(PageTableFlag::NX,            noExecute);
    pageMapLevelFour->entries[indexer.page_directory_pointer()] = PDE;
    PDP = (PageTable*)PDE.address();

    PDE = PDP->entries[indexer.page_directory()];
    PageTable* PD;
    if (!PDE.flag(PageTableFlag::Present)) {
        PD = (PageTable*)request_page();
        memset(PD, 0, PAGE_SIZE);
        PDE.set_address((u64)PD);
    }
    PDE.or_flag_if(PageTableFlag::Present, present);
    PDE.or_flag_if(PageTableFlag::ReadWrite, write);
    PDE.or_flag_if(PageTableFlag::UserSuper, user);
    // PDE.or_flag_if(PageTableFlag::NX,            noExecute);
    PDP->entries[indexer.page_directory()] = PDE;
    PD = (PageTable*)PDE.address();

    PDE = PD->entries[indexer.page_table()];
    PageTable* PT;
    // Split large 2MiB page if necessary
    if (PDE.flag(PageTableFlag::Present) and PDE.flag(PageTableFlag::LargerPages)) {
        if (debug == ShowDebug::Yes or true)
            std::print("[Page Splitting] Splitting 2MiB large page at {} into 4KiB pages\n", virtualAddress);

        // Allocate a Page Table Level 1
        PT = (PageTable*)request_page();
        memset(PT, 0, PAGE_SIZE);

        // Extract the physical base address of the 2MiB page
        u64 physical_base = PDE.address_large();

        // Backfill all entries of the new Page Table Level 1 to exactly match the
        // 2MiB layout.
        const u64 target_i = (((uintptr_t)virtualAddress) >> 12) & 0x1ff;
        for (u64 i = 0; i < 512; ++i) {
            PageDirectoryEntry SubPDE{};

            // Map the sub-page physical address (base + i * 4KiB)
            SubPDE.set_address(physical_base + (i * PAGE_SIZE));

            // Copy all original permission attributes down to the sub-pages, except
            // for our split page.
            if (i == target_i) {
                // new address
                SubPDE.set_address((uintptr_t)physicalAddress);
                // new flags
                SubPDE.set_flag(PageTableFlag::Present, present);
                SubPDE.set_flag(PageTableFlag::ReadWrite, write);
                SubPDE.set_flag(PageTableFlag::UserSuper, user);
                SubPDE.set_flag(PageTableFlag::WriteThrough, writeThrough);
                SubPDE.set_flag(PageTableFlag::CacheDisabled, cacheDisabled);
                SubPDE.set_flag(PageTableFlag::Global, global);
            }
            else {
                SubPDE.or_flag_if(PageTableFlag::Present, PDE.flag(PageTableFlag::Present));
                SubPDE.or_flag_if(PageTableFlag::ReadWrite, PDE.flag(PageTableFlag::ReadWrite));
                SubPDE.or_flag_if(PageTableFlag::UserSuper, PDE.flag(PageTableFlag::UserSuper));
                SubPDE.or_flag_if(PageTableFlag::WriteThrough, PDE.flag(PageTableFlag::WriteThrough));
                SubPDE.or_flag_if(PageTableFlag::CacheDisabled, PDE.flag(PageTableFlag::CacheDisabled));
                SubPDE.or_flag_if(PageTableFlag::Global, PDE.flag(PageTableFlag::Global));
            }
            PT->entries[i] = SubPDE;
        }

        // Disable Large Page
        PDE.set_flag(PageTableFlag::LargerPages, false);
        // Clear out 2MiB physical memory address, point it to new Page Table
        // Level 1.
        PDE.set_address((u64)PT);

        // Commit it back to Level 2
        PD->entries[indexer.page_table()] = PDE;
        return;
    }
    if (!PDE.flag(PageTableFlag::Present)) {
        PT = (PageTable*)request_page();
        memset(PT, 0, PAGE_SIZE);
        PDE.set_address((u64)PT);
    }
    PDE.or_flag_if(PageTableFlag::Present, present);
    PDE.or_flag_if(PageTableFlag::ReadWrite, write);
    PDE.or_flag_if(PageTableFlag::UserSuper, user);
    // PDE.or_flag_if(PageTableFlag::NX,            noExecute);
    PD->entries[indexer.page_table()] = PDE;
    PT = (PageTable*)PDE.address();

    PDE = PT->entries[indexer.page()];
    PDE.set_address((u64)physicalAddress);
    PDE.set_flag(PageTableFlag::Present, present);
    PDE.set_flag(PageTableFlag::ReadWrite, write);
    PDE.set_flag(PageTableFlag::UserSuper, user);
    PDE.set_flag(PageTableFlag::WriteThrough, writeThrough);
    PDE.set_flag(PageTableFlag::CacheDisabled, cacheDisabled);
    PDE.set_flag(PageTableFlag::Global, global);
    // PDE.set_flag(PageTableFlag::NX,            noExecute);
    PT->entries[indexer.page()] = PDE;
    if (debug == ShowDebug::Yes)
        std::print("  {Mapped}\n\n", __GREEN);
}

void map(void* virtualAddress, void* physicalAddress, u64 mappingFlags, ShowDebug debug) {
    map(active_page_map(), virtualAddress, physicalAddress, mappingFlags, debug);
}

void map_pages(PageTable* pageTable, void* virtualAddress, void* physicalAddress, u64 mappingFlags, usz pageCount, ShowDebug d) {
    // We can't name this virtual because it's a keyword.
    u64 virt = u64(virtualAddress);
    u64 physical = u64(physicalAddress);
    for (u64 end = virt + (pageCount * PAGE_SIZE); virt < end; virt += PAGE_SIZE, physical += PAGE_SIZE) {
        Memory::map(pageTable, (void*)virt, (void*)physical, mappingFlags, d);
    }
}
void map_pages(void* virtualAddress, void* physicalAddress, u64 mappingFlags, usz pageCount, ShowDebug d) {
    Memory::map_pages(active_page_map(), virtualAddress, physicalAddress, mappingFlags, pageCount, d);
}

void map_large(PageTable* pageMapLevelFour, void* virtualAddress, void* physicalAddress, u64 mappingFlags, ShowDebug debug) {
    if (pageMapLevelFour == nullptr)
        return;

    PageMapIndexer indexer((u64)virtualAddress);
    PageDirectoryEntry PDE;

    bool present = mappingFlags & static_cast<u64>(PageTableFlag::Present);
    bool write = mappingFlags & static_cast<u64>(PageTableFlag::ReadWrite);
    bool user = mappingFlags & static_cast<u64>(PageTableFlag::UserSuper);
    bool writeThrough = mappingFlags & static_cast<u64>(PageTableFlag::WriteThrough);
    bool cacheDisabled = mappingFlags & static_cast<u64>(PageTableFlag::CacheDisabled);
    bool global = mappingFlags & static_cast<u64>(PageTableFlag::Global);

    if (debug == ShowDebug::Yes) {
        std::print(
            "Attempting to map large 2MiB virtual {} to physical {} in page directory at {}\n"
            "  Flags:\n"
            "    Present:         {}\n"
            "    Write:           {}\n"
            "    User-accessible: {}\n"
            "    Write-through:   {}\n"
            "    Cache Disabled:  {}\n"
            "    Global:          {}\n"
            "\n",
            virtualAddress,
            physicalAddress,
            (void*)pageMapLevelFour,
            present,
            write,
            user,
            writeThrough,
            cacheDisabled,
            global);
    }

    // Page Map Level 4 -> Page Directory Pointer Table Level 3
    PDE = pageMapLevelFour->entries[indexer.page_directory_pointer()];
    PageTable* PDP;
    if (!PDE.flag(PageTableFlag::Present)) {
        PDP = (PageTable*)request_page();
        memset(PDP, 0, PAGE_SIZE);
        PDE.set_address((u64)PDP);
    }
    PDE.or_flag_if(PageTableFlag::Present, present);
    PDE.or_flag_if(PageTableFlag::ReadWrite, write);
    PDE.or_flag_if(PageTableFlag::UserSuper, user);
    pageMapLevelFour->entries[indexer.page_directory_pointer()] = PDE;
    PDP = (PageTable*)PDE.address();

    // Page Directory Pointer Table Level 3 -> Page Directory Level 2
    PDE = PDP->entries[indexer.page_directory()];
    PageTable* PD;
    if (!PDE.flag(PageTableFlag::Present)) {
        PD = (PageTable*)request_page();
        memset(PD, 0, PAGE_SIZE);
        PDE.set_address((u64)PD);
    }
    PDE.or_flag_if(PageTableFlag::Present, present);
    PDE.or_flag_if(PageTableFlag::ReadWrite, write);
    PDE.or_flag_if(PageTableFlag::UserSuper, user);
    PDP->entries[indexer.page_directory()] = PDE;
    PD = (PageTable*)PDE.address();

    // Page Directory Level 2 -> Page Directory Entry (large)
    PDE = PD->entries[indexer.page_table()];
    PDE.set_address_large((u64)physicalAddress);
    PDE.or_flag_if(PageTableFlag::Present, present);
    PDE.or_flag_if(PageTableFlag::ReadWrite, write);
    PDE.or_flag_if(PageTableFlag::UserSuper, user);
    PDE.or_flag_if(PageTableFlag::WriteThrough, writeThrough);
    PDE.or_flag_if(PageTableFlag::CacheDisabled, cacheDisabled);
    PDE.set_flag(PageTableFlag::LargerPages, true); /** (!) **/
    PDE.or_flag_if(PageTableFlag::Global, global);
    PD->entries[indexer.page_table()] = PDE;

    if (debug == ShowDebug::Yes)
        std::print("  {Mapped} (2MiB large page)\n\n", __GREEN);
}

void unmap(PageTable* pageMapLevelFour, void* virtualAddress, ShowDebug debug) {
    if (debug == ShowDebug::Yes)
        std::print("Attempting to unmap virtual {} in page table at {}\n", virtualAddress, (void*)pageMapLevelFour);

    PageMapIndexer indexer((u64)virtualAddress);
    PageDirectoryEntry PDE;

    // Page Map Level 4 -> Page Directory Pointer Table Level 3
    PDE = pageMapLevelFour->entries[indexer.page_directory_pointer()];
    if (not PDE.flag(PageTableFlag::Present))  // Already unmapped.
        return;
    auto* PDP = (PageTable*)PDE.address();

    // Page Directory Pointer Table Level 3 -> Page Directory Level 2
    PDE = PDP->entries[indexer.page_directory()];
    if (not PDE.flag(PageTableFlag::Present))  // Already unmapped.
        return;
    // 1GiB Large Pages
    if (PDE.flag(PageTableFlag::LargerPages)) {
        PDE.set_flag(PageTableFlag::Present, false);
        PDE.set_flag(PageTableFlag::LargerPages, false);
        PDP->entries[indexer.page_directory()] = PDE;
        if (debug == ShowDebug::Yes)
            std::print("  \033[32mUnmapped\033[0m (1GiB large page)\n\n");
        return;
    }
    auto* PD = (PageTable*)PDE.address();

    // Page Directory Level 2 -> Page Table Level 1
    PDE = PD->entries[indexer.page_table()];
    // 2MiB Large Pages
    if (PDE.flag(PageTableFlag::LargerPages)) {
        PDE.set_flag(PageTableFlag::Present, false);
        PDE.set_flag(PageTableFlag::LargerPages, false);
        PD->entries[indexer.page_table()] = PDE;
        if (debug == ShowDebug::Yes)
            std::print("  \033[32mUnmapped\033[0m (2MiB large page)\n\n");
        return;
    }
    auto* PT = (PageTable*)PDE.address();

    // Page Table Level 1 -> Page Table Entry
    PDE = PT->entries[indexer.page()];
    PDE.set_flag(PageTableFlag::Present, false);
    PT->entries[indexer.page()] = PDE;
    if (debug == ShowDebug::Yes)
        std::print("  \033[32mUnmapped\033[0m\n\n");
}

void unmap(void* virtualAddress, ShowDebug d) {
    unmap(ActivePageMap, virtualAddress, d);
}

void unmap_pages(PageTable* pageTable, void* virtualAddress, usz pageCount, ShowDebug d) {
    if (d == Memory::ShowDebug::Yes) {
        std::print("Attempting to unmap {} pages starting at virtual {} in page table at {}\n", pageCount, virtualAddress, (void*)pageTable);
    }
    u64 end = u64(virtualAddress) + (pageCount * PAGE_SIZE);
    for (u64 t = u64(virtualAddress); t < end; t += PAGE_SIZE) {
        Memory::unmap(pageTable, (void*)t, d);
    }
}

Memory::PageTable* clone_page_map_copy_on_write(Memory::PageTable* oldPageTable) {
    auto make_pde_cow = [](Memory::PageDirectoryEntry& pde) {
        if (pde.flag(Memory::PageTableFlag::ReadWrite)) {
            // Unset write flag; this means any writes to this page will cause a page fault.
            pde.set_flag(Memory::PageTableFlag::ReadWrite, false);
            // Set the "copy on write" flag. This will allow the page fault to detect that it
            // should actually copy this region, perform the write or whatever, and then return.
            pde.set_flag(Memory::PageTableFlag::Lensor_CopyOnWrite, true);
        }
    };

    // FIXME: Free already allocated pages upon failure.
    Memory::PageDirectoryEntry PDE;

    auto* newPageTable = reinterpret_cast<Memory::PageTable*>(Memory::request_page());
    if (newPageTable == nullptr) {
        std::print("Failed to allocate memory for new process page map level four.\n");
        return nullptr;
    }
    memset(newPageTable, 0, PAGE_SIZE);
    for (u64 i = 0; i < 512; ++i) {
        PDE = oldPageTable->entries[i];
        if (PDE.flag(Memory::PageTableFlag::Present) == false)
            continue;

        auto* newPDP = (Memory::PageTable*)Memory::request_page();
        if (newPDP == nullptr) {
            std::print("Failed to allocate memory for new process page directory pointer table.\n");
            return nullptr;
        }
        memset(newPDP, 0, PAGE_SIZE);
        auto* oldTable = (Memory::PageTable*)PDE.address();
        for (u64 j = 0; j < 512; ++j) {
            PDE = oldTable->entries[j];
            if (PDE.flag(Memory::PageTableFlag::Present) == false)
                continue;

            auto* newPD = (Memory::PageTable*)Memory::request_page();
            if (newPD == nullptr) {
                std::print("Failed to allocate memory for new process page directory table.\n");
                return nullptr;
            }
            memset(newPD, 0, PAGE_SIZE);
            auto* oldPD = (Memory::PageTable*)PDE.address();
            for (u64 k = 0; k < 512; ++k) {
                PDE = oldPD->entries[k];
                if (PDE.flag(Memory::PageTableFlag::Present) == false)
                    continue;

                auto* newPT = (Memory::PageTable*)Memory::request_page();
                if (newPT == nullptr) {
                    std::print("Failed to allocate memory for new process page table.\n");
                    return nullptr;
                }
                memset(newPT, 0, PAGE_SIZE);
                auto* oldPT = (Memory::PageTable*)PDE.address();
                // memcpy(newPT, oldPT, PAGE_SIZE);
                for (u64 l = 0; l < 512; ++l) {
                    PDE = oldPT->entries[l];
                    if (PDE.flag(Memory::PageTableFlag::Present) == false)
                        continue;

                    make_pde_cow(PDE);
                    newPT->entries[l] = PDE;
                }
                PDE = oldPD->entries[k];
                PDE.set_address((u64)newPT);
                make_pde_cow(PDE);
                newPD->entries[k] = PDE;
            }
            PDE = oldTable->entries[j];
            PDE.set_address((u64)newPD);
            make_pde_cow(PDE);
            newPDP->entries[j] = PDE;
        }
        PDE = oldPageTable->entries[i];
        PDE.set_address((u64)newPDP);
        make_pde_cow(PDE);
        newPageTable->entries[i] = PDE;
    }
    return newPageTable;
}

Memory::PageTable* clone_page_map(Memory::PageTable* oldPageTable) {
    // FIXME: Free already allocated pages upon failure.
    Memory::PageDirectoryEntry PDE;

    auto* newPageTable = reinterpret_cast<Memory::PageTable*>(Memory::request_page());

    if (newPageTable == nullptr) {
        std::print("Failed to allocate memory for new process page map level four.\n");
        return nullptr;
    }
    memset(newPageTable, 0, PAGE_SIZE);
    for (u64 i = 0; i < 512; ++i) {
        PDE = oldPageTable->entries[i];
        if (PDE.flag(Memory::PageTableFlag::Present) == false)
            continue;

        // Copy higher-half mappings exactly...
        if (i >= 256) {
            newPageTable->entries[i] = PDE;
            continue;
        }

        auto* newPDP = (Memory::PageTable*)Memory::request_page();
        if (newPDP == nullptr) {
            std::print("Failed to allocate memory for new process page directory pointer table.\n");
            return nullptr;
        }
        memset(newPDP, 0, PAGE_SIZE);
        auto* oldPDP = (Memory::PageTable*)PDE.address();
        for (u64 j = 0; j < 512; ++j) {
            PDE = oldPDP->entries[j];
            if (PDE.flag(Memory::PageTableFlag::Present) == false)
                continue;

            auto* newPD = (Memory::PageTable*)Memory::request_page();
            if (newPD == nullptr) {
                std::print("Failed to allocate memory for new process page directory table.\n");
                return nullptr;
            }
            memset(newPD, 0, PAGE_SIZE);
            auto* oldPD = (Memory::PageTable*)PDE.address();
            for (u64 k = 0; k < 512; ++k) {
                PDE = oldPD->entries[k];
                if (PDE.flag(Memory::PageTableFlag::Present) == false)
                    continue;

                if (PDE.flag(Memory::PageTableFlag::LargerPages)) {
                    // This is a 2MiB mapping! No Level 1 table exists.
                    // Simply copy the entire large page entry descriptor over to the new directory.
                    newPD->entries[k] = PDE;
                    continue;
                }

                auto* newPT = (Memory::PageTable*)Memory::request_page();
                if (newPT == nullptr) {
                    std::print("Failed to allocate memory for new process page table.\n");
                    return nullptr;
                }
                memset(newPT, 0, PAGE_SIZE);
                auto* oldPT = (Memory::PageTable*)PDE.address();
                // memcpy(newPT, oldPT, PAGE_SIZE);
                for (u64 l = 0; l < 512; ++l) {
                    PDE = oldPT->entries[l];
                    if (PDE.flag(Memory::PageTableFlag::Present) == false)
                        continue;

                    newPT->entries[l] = PDE;
                }
                PDE = oldPD->entries[k];
                PDE.set_address((u64)newPT);
                newPD->entries[k] = PDE;
            }
            PDE = oldPDP->entries[j];
            PDE.set_address((u64)newPD);
            newPDP->entries[j] = PDE;
        }
        PDE = oldPageTable->entries[i];
        PDE.set_address((u64)newPDP);
        newPageTable->entries[i] = PDE;
    }
    return newPageTable;
}

void free_page_map(PageTable* pageTable) {
    if (pageTable == nullptr) {
        std::print("[VIRT]: Cannot free NULL page table...\n");
        return;
    }
    if (pageTable == active_page_map()) {
        std::print("[VIRT]: Cannot free currently active page table...\n");
        return;
    }
    PageDirectoryEntry PDE;
    for (u64 i = 0; i < 512; ++i) {
        // std::print("  PDP {}\n", i);
        PDE = pageTable->entries[i];
        if (!PDE.flag(PageTableFlag::Present))
            continue;

        auto* PDP = (PageTable*)PDE.address();
        // For some reason a ton of these addresses have all garbage in them...
        if ((usz)PDP == 0x000ffffffffff000 || (usz)PDP > Memory::total_ram() || (usz)PDP % PAGE_SIZE != 0)
            continue;

        // std::print("  PDP {} present at {}\n", i, (void*)PDP);
        for (u64 j = 0; j < 512; ++j) {
            // std::print("  PD {}\n", j);
            PDE = PDP->entries[j];
            if (!PDE.flag(PageTableFlag::Present))
                continue;

            auto* PD = (PageTable*)PDE.address();
            if ((usz)PD == 0x000ffffffffff000 || (usz)PD > Memory::total_ram() || (usz)PD % PAGE_SIZE != 0)
                continue;

            // std::print("  PD {} present at {}\n", j, (void*)PD);
            for (u64 k = 0; k < 512; ++k) {
                // std::print("  PT {}\n", k);
                PDE = PD->entries[k];
                if (!PDE.flag(PageTableFlag::Present))
                    continue;

                auto* PT = (PageTable*)PDE.address();
                if ((usz)PT == 0x000ffffffffff000 || (usz)PT > Memory::total_ram() || (usz)PT % PAGE_SIZE != 0)
                    continue;

                // std::print("  PT {} present at {}\n", k, (void*)PT);
                free_page(PT);
            }
            free_page(PD);
        }
        free_page(PDP);
    }
    free_page(pageTable);
}

PageTable* active_page_map() {
    if (!ActivePageMap) {
        asm volatile(
            "mov %%cr3, %%rax\n\t"
            "mov %%rax, %0"
            : "=m"(ActivePageMap)
            :  // No inputs
            : "rax");
    }
    return ActivePageMap;
}

PageTable* clone_active_page_map() {
    return clone_page_map(active_page_map());
}

void init_virtual(PageTable* pageMap) {
    /* Map all physical RAM addresses to virtual addresses 1:1,
     * store them in the PML4. This means that virtual memory
     * addresses will be equal to physical memory addresses within
     * the kernel.
     */
    for (u64 t = 0; t < total_ram(); t += PAGE_SIZE_LARGE) {
        map_large(
            pageMap,
            (void*)(t + Memory::PHYSICAL_BASE),
            (void*)t,
            (u64)PageTableFlag::Present | (u64)PageTableFlag::ReadWrite,
            ShowDebug::No);
    }
    u64 kPhysicalStart = (u64)&KERNEL_PHYSICAL;
    u64 kernelBytesNeeded = 1 + ((u64)&KERNEL_END - (u64)&KERNEL_START);
    for (u64 t = kPhysicalStart; t < kPhysicalStart + kernelBytesNeeded + PAGE_SIZE; t += PAGE_SIZE) {
        map(
            pageMap,
            (void*)(t + (u64)&KERNEL_VIRTUAL),
            (void*)t,
            (u64)PageTableFlag::Present | (u64)PageTableFlag::ReadWrite);
    }
    // Make null-dereference generate exception.
    // unmap(nullptr);
    // Update current page map.
    flush_page_map(pageMap);
}

void init_virtual() {
    Memory::PageTable* table = (PageTable*)Memory::request_page();
    memset(table, 0, PAGE_SIZE);
    init_virtual(table);
}

void print_page_map(Memory::PageTable* oldPageTable, Memory::PageTableFlag filter) {
    // Lambda to avoid duplicating the printing block
    auto print_range = [&](u64 start, u64 end, u64 currentFlags) {
        if (currentFlags != -1ull
            and (currentFlags & (u64)Memory::PageTableFlag::Present)
            and (currentFlags & (u64)filter) == (u64)filter) {
            std::print("Present: {:#016x} to {:#016x} |", start, end);
            if (currentFlags & (u64)Memory::PageTableFlag::ReadWrite) std::print(" RW");
            if (currentFlags & (u64)Memory::PageTableFlag::UserSuper) std::print(" US");
            if (currentFlags & (u64)Memory::PageTableFlag::WriteThrough) std::print(" WT");
            if (currentFlags & (u64)Memory::PageTableFlag::CacheDisabled) std::print(" CD");
            if (currentFlags & (u64)Memory::PageTableFlag::Accessed) std::print(" AC");
            if (currentFlags & (u64)Memory::PageTableFlag::Dirty) std::print(" DT");
            if (currentFlags & (u64)Memory::PageTableFlag::LargerPages) std::print(" LG");
            if (currentFlags & (u64)Memory::PageTableFlag::Global) std::print(" GB");
            if (currentFlags & (u64)Memory::PageTableFlag::NX) std::print(" NX");
            std::print("\n");
        }
    };

    u64 startAddress = -1ull;
    u64 endAddress = -1ull;
    u64 flags = -1ull;
    Memory::PageDirectoryEntry PDE;
    for (u64 i = 0; i < 512; ++i) {
        PDE = oldPageTable->entries[i];
        if (PDE.flag(Memory::PageTableFlag::Present) == false)
            continue;

        auto* oldTable = (Memory::PageTable*)(PDE.address());
        for (u64 j = 0; j < 512; ++j) {
            PDE = oldTable->entries[j];
            if (PDE.flag(Memory::PageTableFlag::Present) == false)
                continue;

            auto* oldPD = (Memory::PageTable*)(PDE.address());
            for (u64 k = 0; k < 512; ++k) {
                PDE = oldPD->entries[k];
                if (PDE.flag(Memory::PageTableFlag::Present) == false)
                    continue;

                if (PDE.flag(PageTableFlag::LargerPages)) {
                    u64 virtualAddress = 0;
                    virtualAddress |= i << 27;
                    virtualAddress |= j << 18;
                    virtualAddress |= k << 9;
                    virtualAddress <<= 12;

                    // Sign-extension for canonical addresses
                    if (virtualAddress & (1ULL << 47))
                        virtualAddress |= 0xffff000000000000ull;

                    endAddress = virtualAddress;

                    if (flags != -1ull && PDE.flags() != flags) {
                        print_range(startAddress, endAddress, flags);
                        startAddress = virtualAddress;
                        flags = PDE.flags();
                    }

                    if (startAddress == -1ull) startAddress = virtualAddress;
                    if (flags == -1ull) flags = PDE.flags();

                    continue;
                }

                auto* oldPT = (Memory::PageTable*)(PDE.address());
                for (u64 l = 0; l < 512; ++l) {
                    PDE = oldPT->entries[l];

                    // Virtual Address from indices
                    u64 virtualAddress = 0;
                    virtualAddress |= i << 27;
                    virtualAddress |= j << 18;
                    virtualAddress |= k << 9;
                    virtualAddress |= l << 0;
                    virtualAddress <<= 12;

                    // Sign-extension for canonical addresses
                    if (virtualAddress & (1ULL << 47))
                        virtualAddress |= 0xffff000000000000ull;

                    endAddress = virtualAddress;

                    // If flags does not equal new flags, stop and print.
                    if (flags != -1ull && PDE.flags() != flags) {
                        print_range(startAddress, endAddress, flags);
                        startAddress = endAddress;
                        flags = PDE.flags();
                        continue;
                    }

                    if (startAddress == -1ull)
                        startAddress = endAddress;

                    if (flags == -1ull)
                        flags = PDE.flags();
                }
            }
        }
    }
    print_range(startAddress, endAddress, flags);
}

void print_pde_flags(Memory::PageDirectoryEntry PDE) {
    if (PDE.flag(Memory::PageTableFlag::ReadWrite))
        std::print(" RW");
    if (PDE.flag(Memory::PageTableFlag::UserSuper))
        std::print(" US");
    if (PDE.flag(Memory::PageTableFlag::WriteThrough))
        std::print(" WT");
    if (PDE.flag(Memory::PageTableFlag::CacheDisabled))
        std::print(" CD");
    if (PDE.flag(Memory::PageTableFlag::Accessed))
        std::print(" AC");
    if (PDE.flag(Memory::PageTableFlag::Dirty))
        std::print(" DT");
    if (PDE.flag(Memory::PageTableFlag::LargerPages))
        std::print(" LG");
    if (PDE.flag(Memory::PageTableFlag::Global))
        std::print(" GB");
    if (PDE.flag(Memory::PageTableFlag::NX))
        std::print(" NX");
}

}  // namespace Memory

extern "C" void flush_page_map(Memory::PageTable* pageMapLevelFour) {
    asm volatile("mov %0, %%cr3"
                 :  // No outputs
                 : "r"(pageMapLevelFour)
                 : "memory");
    Memory::ActivePageMap = pageMapLevelFour;
}
