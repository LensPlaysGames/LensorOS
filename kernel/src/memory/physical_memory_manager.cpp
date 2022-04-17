#include <memory/physical_memory_manager.h>

#include <bitmap.h>
#include <cstr.h>
#include <debug.h>
#include <efi_memory.h>
#include <link_definitions.h>
#include <memory/region.h>
#include <memory/common.h>
#include <memory/virtual_memory_manager.h>
#include <panic.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_PMM

namespace Memory {
    Bitmap PageMap;

    u64 TotalPages { 0 };
    u64 TotalFreePages { 0 };
    u64 TotalUsedPages { 0 };
    u64 MaxFreePagesInARow { 0 };

    u64 total_ram() {
        return TotalPages * PAGE_SIZE;
    }
    u64 free_ram() {
        return TotalFreePages * PAGE_SIZE;
    }
    u64 used_ram() {
        return TotalUsedPages * PAGE_SIZE;
    }

    void lock_page(void* address) {
        u64 index = (u64)address / PAGE_SIZE;
        // Page already locked.
        if (PageMap.get(index))
            return;

        if (PageMap.set(index, true)) {
            TotalFreePages -= 1;
            TotalUsedPages += 1;
        }
    }

    void lock_pages(void* address, u64 numberOfPages) {
        for (u64 i = 0; i < numberOfPages; ++i)
            lock_page((void*)((u64)address + (i * PAGE_SIZE)));
    }

    void free_page(void* address) {
        u64 index = (u64)address / PAGE_SIZE;
        if (PageMap.get(index) == false)
            return;

        if (PageMap.set(index, false)) {
            TotalUsedPages -= 1;
            TotalFreePages += 1;
        }
    }

    void free_pages(void* address, u64 numberOfPages) {
#ifdef DEBUG_PMM
        dbgmsg("free_pages():\r\n"
               "  Address:     %x\r\n"
               "  # of pages:  %ull\r\n"
               "  Free before: %ull\r\n"
               , address
               , numberOfPages
               , TotalFreePages);
#endif
        for (u64 i = 0; i < numberOfPages; ++i)
            free_page((void*)((u64)address + (i * PAGE_SIZE)));

#ifdef DEBUG_PMM
        dbgmsg("  Free after: %ull\r\n"
               "\r\n"
               , TotalFreePages);
#endif /* defined DEBUG_PMM */
    }

    u64 FirstFreePage { 0 };
    void* request_page() {
#ifdef DEBUG_PMM
        dbgmsg("request_page():\r\n"
               "  Free pages:            %ull\r\n"
               "  Max run of free pages: %ull\r\n"
               "\r\n"
               , TotalFreePages
               , MaxFreePagesInARow);
#endif
        for(; FirstFreePage < TotalPages; FirstFreePage++) {
            if (PageMap.get(FirstFreePage) == false) {
                void* addr = (void*)(FirstFreePage * PAGE_SIZE);
                lock_page(addr);
                FirstFreePage += 1; // Eat current page.
#ifdef DEBUG_PMM
                dbgmsg("  Successfully fulfilled memory request: %x\r\n"
                       "\r\n", addr);
#endif
                return addr;
            }
        }
        // TODO: Page swap from/to file on disk.
        panic("\033[31mRan out of memory in request_page() :^<\033[0m\r\n");
        return nullptr;
    }
    
    void* request_pages(u64 numberOfPages) {
        // Can't allocate nothing!
        if (numberOfPages == 0)
            return nullptr;
        // One page is easier to allocate than a run of contiguous pages.
        if (numberOfPages == 1)
            return request_page();
        // Can't allocate something larger than the amount of free memory.
        if (numberOfPages > TotalFreePages) {
            dbgmsg("request_pages(): \033[31mERROR\033[0m:: "
                   "Number of pages requested is larger than amount of pages available.");
            return nullptr;
        }
        if (numberOfPages > MaxFreePagesInARow) {
            dbgmsg("request_pages(): \033[31mERROR\033[0m:: "
                   "Number of pages requested is larger than any contiguous run of pages available."
                   );
            return nullptr;
        }
        
#ifdef DEBUG_PMM
        dbgmsg("request_pages():\r\n"
               "  # of pages requested:  %ull\r\n"
               "  Free pages:            %ull\r\n"
               "  Max run of free pages: %ull\r\n"
               "\r\n"
               , numberOfPages
               , TotalFreePages
               , MaxFreePagesInARow);
#endif

        for (u64 i = FirstFreePage; i < PageMap.length(); ++i) {
            // Skip locked pages.
            if (PageMap[i] == true)
                continue;

            // If page is free, check if entire `numberOfPages` run is free.
            u64 index = i;
            u64 run = 0;

            while (PageMap[index] == false) {
                run++;
                index++;

                if (index >= PageMap.length()) {
                    // TODO: No memory matching criteria, should
                    //   probably do a page swap from disk or something.
                    panic("\033[0mRan out of memory in request_pages() :^<\033[0m\r\n");
                    return nullptr;
                }
                if (run >= numberOfPages) {
                    void* out = (void*)(i * PAGE_SIZE);
                    lock_pages(out, numberOfPages);
#ifdef DEBUG_PMM
                    dbgmsg("  Successfully fulfilled memory request: %x\r\n"
                           "\r\n", out);
#endif
                    return out;
                }
            }
            // If this point is reached, it means run was not long enough.
            // Start searching for next run after the run we've already determined is not long enough.
            i = index;
        }
        return nullptr;
    }

    constexpr u64 InitialPageBitmapMaxAddress = MiB(64);
    constexpr u64 InitialPageBitmapPageCount = InitialPageBitmapMaxAddress / PAGE_SIZE;
    constexpr u64 InitialPageBitmapSize = InitialPageBitmapPageCount / 8;
    u8 InitialPageBitmap[InitialPageBitmapSize];

    void init_physical(EFI_MEMORY_DESCRIPTOR* memMap, u64 size, u64 entrySize) {
        // Calculate number of entries within memoryMap array.
        u64 entries = size / entrySize;
        // Find largest free and usable contiguous region of memory
        // within space addressable by initial page bitmap.
        void* largestFreeMemorySegment { nullptr };
        u64 largestFreeMemorySegmentPageCount { 0 };
        for (u64 i = 0; i < entries; ++i) {
            EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)memMap + (i * entrySize));
            if (desc->type == 7) {
                if (desc->numPages > largestFreeMemorySegmentPageCount
                    && (u64)desc->physicalAddress + desc->numPages < InitialPageBitmapMaxAddress)
                {
                    largestFreeMemorySegment = desc->physicalAddress;
                    largestFreeMemorySegmentPageCount = desc->numPages;
                }
            }
            TotalPages += desc->numPages;
        }

        if (largestFreeMemorySegment == nullptr
            || largestFreeMemorySegmentPageCount == 0)
        {
            while (true)
                asm ("hlt");
        }

        // Use pre-allocated memory region for initial physical page bitmap.
        PageMap.init(InitialPageBitmapSize, (u8*)&InitialPageBitmap[0]);
        // Lock all pages in initial bitmap.
        lock_pages(0, InitialPageBitmapPageCount);
        // Unlock free pages in bitmap.
        for (u64 i = 0; i < entries; ++i) {
            EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)memMap + (i * entrySize));
            if (desc->type == 7)
                free_pages(desc->physicalAddress, desc->numPages);
        }
        // Lock the kernel (in case it was just freed).
        u64 kernelByteCount = (u64)&KERNEL_END - (u64)&KERNEL_START;
        u64 kernelPageCount = kernelByteCount / PAGE_SIZE;
        lock_pages(&KERNEL_PHYSICAL, kernelPageCount);
        
        // Use the initial pre-allocated page bitmap as a guide
        // for where to place allocate new virtual memory map entries.
        // Map up to the entire amount of physical memory
        // present or the max amount addressable given the
        // size limitation of the pre-allocated bitmap.
        PageTable* activePML4 = active_page_map();
        for (u64 t = 0; t < TotalPages * PAGE_SIZE && t < InitialPageBitmapMaxAddress; t += 0x1000)
            map(activePML4, (void*)t, (void*)t);

        // Calculate total number of bytes needed for a physical page
        // bitmap that covers hardware's actual amount of memory present.
        u64 bitmapSize = (TotalPages / 8) + 1;
        PageMap.init(bitmapSize, (u8*)((u64)largestFreeMemorySegment));
        TotalUsedPages = 0;
        lock_pages(0, TotalPages + 1);
        // With all pages in the bitmap locked, free only the EFI conventional memory segments.
        // We may be able to be a little more aggressive in what memory we take in the future.
        TotalFreePages = 0;
        for (u64 i = 0; i < entries; ++i) {
            EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)memMap + (i * entrySize));
            if (desc->type == 7) {
                free_pages(desc->physicalAddress, desc->numPages);
                if (desc->numPages > MaxFreePagesInARow)
                    MaxFreePagesInARow = desc->numPages;
            }
        }
        /* The page map itself takes up space within the largest free memory segment.
         * As every memory segment was just set back to free in the bitmap, it's
         *   important to re-lock the page bitmap so it doesn't get trampled on
         *   when allocating more memory.
         */
        lock_pages(PageMap.base(), (PageMap.length() / PAGE_SIZE) + 1);

        // Lock the kernel in the new page bitmap (in case it already isn't).
        lock_pages(&KERNEL_PHYSICAL, kernelPageCount);

        /* TODO:
         * |-- Not everything should be user-accessible!
         * `-- `.text` + `.rodata` should be read only.
         */

        // Calculate space that is lost due to page alignment.
        u64 deadSpace { 0 };
        deadSpace += (u64)&DATA_START - (u64)&TEXT_END;
        deadSpace += (u64)&READ_ONLY_DATA_START - (u64)&DATA_END;
        deadSpace += (u64)&BLOCK_STARTING_SYMBOLS_START - (u64)&READ_ONLY_DATA_END;
        dbgmsg("\033[32m"
               "Physical memory initialized"
               "\033[0m\r\n"
               "  Physical memory mapped from %x thru %x\r\n"
               "  Kernel loaded at %x (%ullMiB)\r\n"
               "  Kernel mapped from %x thru %x (%ullKiB)\r\n"
               "    .text:   %x thru %x (%ull bytes)\r\n"
               "    .data:   %x thru %x (%ull bytes)\r\n"
               "    .rodata: %x thru %x (%ull bytes)\r\n"
               "    .bss:    %x thru %x (%ull bytes)\r\n"
               "    Lost to page alignment: %ull bytes\r\n"
               "\r\n"
               , 0ULL, total_ram()
               , &KERNEL_PHYSICAL, TO_MiB(&KERNEL_PHYSICAL)
               , &KERNEL_START, &KERNEL_END
               , TO_KiB(&KERNEL_END - &KERNEL_START)
               , &TEXT_START, &TEXT_END
               , &TEXT_END - &TEXT_START
               , &DATA_START, &DATA_END
               , &DATA_END - &DATA_START
               , &READ_ONLY_DATA_START, &READ_ONLY_DATA_END
               , &READ_ONLY_DATA_END - &READ_ONLY_DATA_START
               , &BLOCK_STARTING_SYMBOLS_START, &BLOCK_STARTING_SYMBOLS_END
               , &BLOCK_STARTING_SYMBOLS_END - &BLOCK_STARTING_SYMBOLS_START
               , deadSpace
               );
    }

    void print_debug_kib() {
        dbgmsg("Memory Manager Debug Information:\r\n"
               "  Total Memory: %ullKiB\r\n"
               "  Free Memory: %ullKiB\r\n"
               "  Used Memory: %ullKiB\r\n"
               "\r\n"
               , TO_KiB(total_ram())
               , TO_KiB(free_ram())
               , TO_KiB(used_ram())
               );
    }

    void print_debug_mib() {
        dbgmsg("Memory Manager Debug Information:\r\n"
               "  Total Memory: %ullMiB\r\n"
               "  Free Memory: %ullMiB\r\n"
               "  Used Memory: %ullMiB\r\n"
               "\r\n"
               , TO_MiB(total_ram())
               , TO_MiB(free_ram())
               , TO_MiB(used_ram())
               );
    }

    void print_debug() {
        if (total_ram() > MiB(64))
            print_debug_mib();
        else print_debug_kib();
    }
}
