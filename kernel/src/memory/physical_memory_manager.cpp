#include "physical_memory_manager.h"

#include "../bitmap.h"
#include "common.h"
#include "../cstr.h"
#include "../efi_memory.h"
#include "../link_definitions.h"
#include "region.h"
#include "../uart.h"
#include "virtual_memory_manager.h"

namespace Memory {
    Bitmap PageMap;

    u64 TotalPages { 0 };
    u64 TotalFreePages { 0 };
    u64 TotalUsedPages { 0 };
    u64 MaxFreePagesInARow { 0 };

    u64 get_total_ram() {
        return TotalPages * PAGE_SIZE;
    }
    u64 get_free_ram() {
        return TotalFreePages * PAGE_SIZE;
    }
    u64 get_used_ram() {
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
        for (u64 i = 0; i < numberOfPages; ++i)
            free_page((void*)((u64)address + (i * PAGE_SIZE)));
    }

    u64 FirstFreePage { 0 };
    void* request_page() {
        for(; FirstFreePage < TotalPages; FirstFreePage++) {
            if (PageMap.get(FirstFreePage) == false) {
                void* addr = (void*)(FirstFreePage * PAGE_SIZE);
                lock_page(addr);
                FirstFreePage += 1; // Eat current page.
                return addr;
            }
        }
        // TODO: Page swap from/to file on disk.
        UART::out("\033[31mRan out of memory in request_page() :^<\033[0m\r\n");
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
            UART::out("request_pages(): ERROR:: Number of pages requested is larger than amount of pages available.");
            return nullptr;
        }
        if (numberOfPages > MaxFreePagesInARow) {
            UART::out("request_pages(): ERROR:: Number of pages requested is larger than any contiguous run of pages available.");
            return nullptr;
        }
        
        UART::out("request_pages():\r\n  # of Pages: ");
        UART::out(numberOfPages);
        UART::out("\r\n  Free Pages: ");
        UART::out(TotalFreePages);
        UART::out("\r\n  Max Run of Free Pages: ");
        UART::out(MaxFreePagesInARow);
        UART::out("\r\n\r\n");

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
                    UART::out("\033[31mYou ran out of memory in request_pages():^<\033[0m\r\n");
                    UART::out("  Attempted to allocate ");
                    UART::out(numberOfPages);
                    UART::out("  pages\r\n\r\n");
                    return nullptr;
                }
                if (run >= numberOfPages) {
                    void* out = (void*)(i * PAGE_SIZE);
                    lock_pages(out, numberOfPages);
                    return out;
                }
            }
            // If this point is reached, it means run was not long enough.
            // Start searching for next run after the run we've already determined is not long enough.
            i = index;
        }
        return nullptr;
    }

    constexpr u64 InitialPageBitmapMaxAddress = GB(8);
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
        void* kernelAddress = (void*)((u64)&KERNEL_VIRTUAL - (u64)&KERNEL_START);
        u64 kernelByteCount = (u64)&KERNEL_END - (u64)&KERNEL_START;
        u64 kernelPageCount = kernelByteCount / PAGE_SIZE;
        lock_pages(kernelAddress, kernelPageCount);
        
        // Use the initial pre-allocated page map as a place to
        // allocate new virtual memory map entries.
        // Map up to the entire amount of physical memory
        // present or the max amount addressable given the
        // size limitation of the pre-allocated bitmap.
        PageTable* activePML4 = get_active_page_map();
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
        lock_pages((void*)((u64)&KERNEL_START - (u64)&KERNEL_VIRTUAL), kernelPageCount);

        /* The page map itself takes up space within the largest free memory segment.
         * As every memory segment was just set back to free in the bitmap, it's
         *   important to re-lock the page bitmap so it doesn't get trampled on
         *   when allocating more memory.
         */
        lock_pages(PageMap.base(), (PageMap.length() / PAGE_SIZE) + 1);

        // TODO: Re-use memory in-between sections; it's currently wasted.
        // Calculate space that is lost due to page alignment.
        u64 deadSpace { 0 };
        deadSpace += (u64)&DATA_START - (u64)&TEXT_END;
        deadSpace += (u64)&READ_ONLY_DATA_START - (u64)&DATA_END;
        deadSpace += (u64)&BLOCK_STARTING_SYMBOLS_START - (u64)&READ_ONLY_DATA_END;
        UART::out("\033[32mPhysical Memory Initialized\033[0m\r\n  Mapped from 0x");
        UART::out(to_hexstring<u64>(0ULL));
        UART::out(" thru 0x");
        UART::out(to_hexstring<u64>(get_total_ram()));
        UART::out("\r\n  Kernel mapped from 0x");
        UART::out(to_hexstring<void*>(&KERNEL_START));
        UART::out(" to 0x");
        UART::out(to_hexstring<void*>(&KERNEL_END));
        UART::out(" (");
        UART::out((u64)&KERNEL_END - (u64)&KERNEL_START);
        UART::out(" bytes)\r\n    .text:   0x");
        UART::out(to_hexstring<void*>(&TEXT_START));
        UART::out(" thru 0x");
        UART::out(to_hexstring<void*>(&TEXT_END));
        UART::out(" (");
        UART::out((u64)&TEXT_END - (u64)&TEXT_START);
        UART::out(" bytes)\r\n    .data:   0x");
        UART::out(to_hexstring<void*>(&DATA_START));
        UART::out(" thru 0x");
        UART::out(to_hexstring<void*>(&DATA_END));
        UART::out(" (");
        UART::out((u64)&DATA_END - (u64)&DATA_START);
        UART::out(" bytes)\r\n    .rodata: 0x");
        UART::out(to_hexstring<void*>(&READ_ONLY_DATA_START));
        UART::out(" thru 0x");
        UART::out(to_hexstring<void*>(&READ_ONLY_DATA_END));
        UART::out(" (");
        UART::out((u64)&READ_ONLY_DATA_END - (u64)&READ_ONLY_DATA_START);
        UART::out(" bytes)\r\n    .bss:    0x");
        UART::out(to_hexstring<void*>(&BLOCK_STARTING_SYMBOLS_START));
        UART::out(" thru 0x");
        UART::out(to_hexstring<void*>(&BLOCK_STARTING_SYMBOLS_END));
        UART::out(" (");
        UART::out((u64)&BLOCK_STARTING_SYMBOLS_END - (u64)&BLOCK_STARTING_SYMBOLS_START);
        UART::out(" bytes)\r\n");
        UART::out("    Lost to Page Alignment: ");
        UART::out(deadSpace);
        UART::out(" bytes\r\n\r\n");
    }

    void print_debug() {
        UART::out("Memory Manager Debug Dump:\r\n  Total Memory: ");
        UART::out(to_string(TotalPages * PAGE_SIZE / 1024 / 1024));
        UART::out("MiB\r\n  Free Memory: ");
        UART::out(to_string(TotalFreePages * PAGE_SIZE / 1024 / 1024));
        UART::out("MiB\r\n  Used Memory: ");
        UART::out(to_string(TotalUsedPages * PAGE_SIZE / 1024 / 1024));
        UART::out("MiB\r\n\r\n");
    }
}
