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
        // Page was locked by `set()`.
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
        // Page already freed.
        if (PageMap.get(index) == false)
            return;
        // Page was freed by `set()`.
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
        UART::out("Physical page requested\r\n");
        for(; FirstFreePage < TotalPages; FirstFreePage++) {
            UART::out("Checking page ");
            UART::out(FirstFreePage);
            UART::out("\r\n  ");
            UART::out(to_string(PageMap.get(FirstFreePage)));
            if (PageMap.get(FirstFreePage) == false) {
                void* addr = (void*)(FirstFreePage * PAGE_SIZE);
                UART::out("  Got page ");
                UART::out(FirstFreePage);
                UART::out("\r\n");
                lock_page(addr);
                FirstFreePage += 1; // Eat current page.
                return addr;
            }
        }
        // TODO: Page swap from/to file on disk.
        UART::out("\033[31mYou ran out of memory :^<\033[0m\r\n");
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
        if (numberOfPages > TotalFreePages)
            return nullptr;
        
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
                if (index > PageMap.length()) {
                    // TODO: No memory matching criteria, should
                    //   probably do a page swap from disk or something.
                    UART::out("\033[31mYou ran out of memory :^<\033[0m\r\n");
                    return nullptr;
                }
                if (run >= numberOfPages) {
                    void* out = (void*)(i * 4096);
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

    void init_physical_common() {
        u64 kernelPagesNeeded = (((u64)&KERNEL_END - (u64)&KERNEL_START) / 4096) + 1;
        Memory::lock_pages((void*)((u64)&KERNEL_START - (u64)&KERNEL_VIRTUAL), kernelPagesNeeded);
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
    
    // A mebibyte is just enough to address four gibibytes. I feel like that's
    // a good enough amount for an initial bitmap, which, once the largest 
    // memory segment is mapped, can be over-written by an appropriately sized
    // memory bitmap for the total amount of pages given by the EFI memory map.
    // In the future, I should really construct ranges from the 
    // physical/virtual addresses within the memory map entries.
    u8 FourGiBPageBitmap[1048580];

    void init_physical_efi(EFI_MEMORY_DESCRIPTOR* memMap, u64 size, u64 entrySize) {
        // Calculate number of entries within memoryMap array.
        u64 entries = size / entrySize;
        // Find largest free and usable contiguous region of memory.
        void* largestFreeMemorySegment { nullptr };
        u64 largestFreeMemorySegmentPageCount { 0 };
        for (u64 i = 0; i < entries; ++i) {
            EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)memMap + (i * entrySize));
            if (desc->type == 7 && (u64)desc->physicalAddress + desc->numPages < 0x100000 * 8) {
                if (desc->numPages > largestFreeMemorySegmentPageCount) {
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

        UART::out("The largest free memory segment has been found, but it is not yet mapped into virtual address space.\r\n");
        // TODO: Something about it.
        
        // Here's my something 'bout it.
        // A small amount of physical memory can represent
        // a much larger portion of virtual memory.
        // As seen in the `bitmapSize` calculation, the bytes needed is 
        // basically an eighth of the total pages. Each page is 4096 bytes
        // in this implementation, so that's a lot of bang for the buck, so to speak.
        // 4GB virtual is equal to 1 mebibyte of bitmap buffer necessary.
        PageMap.init(1048576 * 8, (u8*)&FourGiBPageBitmap[0]);
        lock_pages(0, 1048576 * 8);
        for (u64 i = 0; i < entries; ++i) {
            EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)memMap + (i * entrySize));
            if (((u64)desc->physicalAddress + desc->numPages) >= 0x100000 * 8)
                continue;

            if (desc->type == 7)
                free_pages(desc->physicalAddress, desc->numPages);
        }
        lock_pages(0, (u64)&KERNEL_END - (u64)&KERNEL_START);

        // Number of bytes needed = (Number of Pages / Bits per Byte) + 1
        u64 bitmapSize = (TotalPages / 8) + 1;
        
        PageTable* activePML4 = get_active_page_map();
        for (u64 t = 0; t < bitmapSize && t < 1048576 * 8; t += 0x1000)
            map(activePML4, (void*)t, (void*)t);

        while (true)
            asm ("hlt");

        PageMap.init(bitmapSize, (u8*)((u64)largestFreeMemorySegment));
        lock_pages(0, TotalPages + 1);
        // With all pages in the bitmap locked, free only the EFI conventional memory segments.
        // We may be able to be a little more aggressive in what memory we take in the future.
        TotalFreePages = 0;
        for (u64 i = 0; i < entries; ++i) {
            EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)memMap + (i * entrySize));
            if (desc->type == 7)
                free_pages(desc->physicalAddress, desc->numPages);
        }

        /* The page map itself takes up space within the largest free memory segment.
         * As every memory segment was just set back to free in the bitmap, it's
         *   important to re-lock the page bitmap so it doesn't get trampled on
         *   when allocating more memory.
         */
        lock_pages(PageMap.base(), (PageMap.length() / 4096) + 1);

        init_physical_common();
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
