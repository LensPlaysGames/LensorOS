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

#include <bitmap.h>
#include <cstr.h>
#include <debug.h>
#include <efi_memory.h>
#include <link_definitions.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <panic.h>

#include <format>

// Uncomment the following directive for extra debug information output.
// #define DEBUG_PMM

#ifdef DEBUG_PMM
#define DBGMSG(...) std::print(__VA_ARGS__)
#else
#define DBGMSG(...)
#endif

namespace Memory {
Bitmap FrameBitmap;

u64 TotalFrameCount{0};
u64 FreeFrameCount{0};
u64 UsedFrameCount{0};
u64 MaxContiguousFreeFrames{0};

u64 FirstFreeFrame{0};

u64 total_ram() {
    return TotalFrameCount * PAGE_SIZE;
}
u64 free_ram() {
    return FreeFrameCount * PAGE_SIZE;
}
u64 used_ram() {
    return UsedFrameCount * PAGE_SIZE;
}

void print_physmem() {
    std::print("PHYSMEM:\n");
    for (usz i = 0; i + 1 < FrameBitmap.length();) {
        bool locked = FrameBitmap.get(i);
        bool last_locked = locked;
        usz begin = i;
        usz run = 1;
        // std::print("  begrun page at {:16x} is locked? {}\n", begin * PAGE_SIZE, last_locked);
        ++i;
        while (i < FrameBitmap.length()) {
            locked = FrameBitmap.get(i);
            if (locked != last_locked) break;
            ++run;
            ++i;
        }
        // std::print("  endrun page at {:16x} is locked? {}\n", i * PAGE_SIZE, locked);
        std::print("  {}: {} pages beginning at {:16x} through {:16x}\n", last_locked ? "used" : "free", run, begin * PAGE_SIZE, (begin + run) * PAGE_SIZE);
    }
}

void lock_page(void* address) {
    u64 index = (u64)address / PAGE_SIZE;
    // Page already locked.
    if (FrameBitmap.get(index))
        return;

    if (FrameBitmap.set(index, true)) {
        FreeFrameCount -= 1;
        UsedFrameCount += 1;
    }
}

void lock_pages(void* address, u64 numberOfPages) {
    for (u64 i = 0; i < numberOfPages; ++i)
        lock_page((void*)((u64)address + (i * PAGE_SIZE)));
}

void free_page_impl(void* address) {
    u64 index = (u64)address / PAGE_SIZE;
    if (FrameBitmap.set(index, false)) {
        if (index < FirstFreeFrame) {
            FirstFreeFrame = index;
        }
        UsedFrameCount -= 1;
        FreeFrameCount += 1;
    }
}

void free_page(void* address) {
    DBGMSG(
        "free_page():\n"
        "  Address:     {}\n"
        "  Free before: {}\n",
        address,
        FreeFrameCount);

    free_page_impl(address);

    DBGMSG(
        "  Free after:  {}\n"
        "\n",
        FreeFrameCount);
}

void free_pages(void* address, u64 numberOfPages) {
    DBGMSG(
        "free_pages():\n"
        "  Address:     {}\n"
        "  # of pages:  {}\n"
        "  Free before: {}\n",
        address,
        numberOfPages,
        FreeFrameCount);
    for (u64 i = 0; i < numberOfPages; ++i)
        free_page_impl((void*)((u64)address + (i * PAGE_SIZE)));

    DBGMSG(
        "  Free after:  {}\n"
        "\n",
        FreeFrameCount);
}

void* request_page() {
    DBGMSG(
        "request_page():\n"
        "  Free pages:            {}\n"
        "  Max run of free pages: {}\n"
        "\n",
        FreeFrameCount,
        MaxContiguousFreeFrames);
    for (; FirstFreeFrame < TotalFrameCount; ++FirstFreeFrame) {
        if (FrameBitmap.get(FirstFreeFrame) == false) {
            void* addr = (void*)(FROM_FRAME_POINTER(FirstFreeFrame * PAGE_SIZE));
            lock_page(addr);
            FirstFreeFrame += 1;  // Eat current page.
            DBGMSG(
                "  Successfully fulfilled memory request: {}\n"
                "\n",
                addr);
            return addr;
        }
    }
    // TODO: Page swap from/to file on disk.
    panic("\033[31mRan out of memory in request_page() :^<\033[0m\n");
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
    if (numberOfPages > FreeFrameCount) {
        std::print(
            "request_pages(): \033[31mERROR\033[0m:: "
            "Number of pages requested is larger than amount of pages available.");
        return nullptr;
    }
    if (numberOfPages > MaxContiguousFreeFrames) {
        std::print(
            "request_pages(): \033[31mERROR\033[0m:: "
            "Number of pages requested is larger than any contiguous run of pages available.");
        return nullptr;
    }

    DBGMSG(
        "request_pages():\n"
        "  # of pages requested:  {}\n"
        "  Free pages:            {}\n"
        "  Max run of free pages: {}\n"
        "\n",
        numberOfPages,
        FreeFrameCount,
        MaxContiguousFreeFrames);

    for (u64 i = FirstFreeFrame; i < FrameBitmap.length(); ++i) {
        // Skip locked pages.
        if (FrameBitmap[i] == true)
            continue;

        // If page is free, check if entire `numberOfPages` run is free.
        u64 index = i;
        u64 run = 0;

        while (FrameBitmap[index] == false) {
            run++;
            index++;

            if (index >= FrameBitmap.length()) {
                // TODO: No memory matching criteria, should
                //   probably do a page swap from disk or something.
                panic("\033[0mRan out of memory in request_pages() :^<\033[0m\n");
                return nullptr;
            }
            if (run >= numberOfPages) {
                void* out = (void*)(FROM_FRAME_POINTER(i * PAGE_SIZE));
                lock_pages(out, numberOfPages);
                DBGMSG(
                    "  Successfully fulfilled memory request: {}\n"
                    "\n",
                    out);
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
    DBGMSG(
        "Attempting to initialize physical memory\n"
        "Searching for largest free contiguous memory region under {:#016x}\n",
        InitialPageBitmapMaxAddress);
    // Calculate number of entries within memoryMap array.
    u64 entries = size / entrySize;
    // Find largest free and usable contiguous region of memory
    // within space addressable by initial page bitmap.
    void* largestFreeMemorySegment{nullptr};
    u64 largestFreeMemorySegmentPageCount{0};
    for (u64 i = 0; i < entries; ++i) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)memMap + (i * entrySize));
        if (desc->type == 7) {
            if (desc->numPages > largestFreeMemorySegmentPageCount
                && (u64)desc->physicalAddress + desc->numPages * PAGE_SIZE < InitialPageBitmapMaxAddress) {
                largestFreeMemorySegment = desc->physicalAddress;
                largestFreeMemorySegmentPageCount = desc->numPages;
            }
        }
        TotalFrameCount += desc->numPages;
    }
    if (largestFreeMemorySegment == nullptr
        || largestFreeMemorySegmentPageCount == 0) {
        std::print(
            "\033[31mERROR:\033[0m "
            "Could not find free memory segment during "
            "physical memory manager intialization.");
        hang();
    }
    DBGMSG(
        "Found initial free memory segment ({}KiB) at {}\n",
        TO_KiB(largestFreeMemorySegmentPageCount * PAGE_SIZE),
        largestFreeMemorySegment);
    // Use pre-allocated memory region for initial physical page bitmap.
    FrameBitmap.init(InitialPageBitmapSize, (u8*)&InitialPageBitmap[0]);
    // Lock all pages in initial bitmap.
    lock_pages(0, InitialPageBitmapPageCount);
    // Unlock free pages in bitmap.
    for (u64 i = 0; i < entries; ++i) {
        auto* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)memMap + (i * entrySize));
        if (desc->type == 7) {
            DBGMSG(
                "Freeing {} pages starting at {:#016x} according to EFI\n",
                desc->numPages,
                (uintptr_t)desc->physicalAddress);
            free_pages(desc->physicalAddress, desc->numPages);
        }
    }
    // Lock the kernel (in case it was just freed).
    usz kernelByteCount = (u64)&KERNEL_END - (u64)&KERNEL_START;
    usz kernelPageCount = kernelByteCount / PAGE_SIZE;
    lock_pages(&KERNEL_PHYSICAL, kernelPageCount);
    // Use the initial pre-allocated page bitmap as a guide
    // for where to allocate new virtual memory map entries.
    // Map up to the entire amount of physical memory
    // present or the max amount addressable given the
    // size limitation of the pre-allocated bitmap.
    // TODO: `.text` + `.rodata` should be read only.
    PageTable* activePML4 = active_page_map();
    for (u64 t = 0;
         t < TotalFrameCount * PAGE_SIZE
         && t < InitialPageBitmapMaxAddress;
         t += PAGE_SIZE) {
        map_large<true>(
            activePML4,
            (void*)(t + PHYSICAL_BASE),
            (void*)t,
            (u64)PageTableFlag::Present
                | (u64)PageTableFlag::ReadWrite,
            ShowDebug::No);
    }
    // Calculate total number of bytes needed for a physical page
    // bitmap that covers hardware's actual amount of memory present.
    u64 bitmapSize = (TotalFrameCount / 8) + 1;
    FrameBitmap.init(bitmapSize, (u8*)((u64)largestFreeMemorySegment + Memory::PHYSICAL_BASE));
    UsedFrameCount = 0;
    lock_pages(0, TotalFrameCount + 1);
    // With all pages in the bitmap locked, free only the EFI conventional memory segments.
    // We may be able to be a little more aggressive in what memory we take in the future.
    FreeFrameCount = 0;
    for (u64 i = 0; i < entries; ++i) {
        auto* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)memMap + (i * entrySize));
        if (desc->type == 7) {
            free_pages(desc->physicalAddress, desc->numPages);
            if (desc->numPages > MaxContiguousFreeFrames)
                MaxContiguousFreeFrames = desc->numPages;
        }
    }
    /* The frame bitmap itself takes up space within the largest free memory segment.
     * As every memory segment was just set back to free in the bitmap, it's
     *   important to re-lock the frame bitmap so it doesn't get trampled on
     *   when allocating more memory.
     */
    lock_pages(FrameBitmap.base(), (FrameBitmap.length() / PAGE_SIZE) + 1);

    // Lock the kernel in the new frame bitmap (in case it already isn't).
    lock_pages(&KERNEL_PHYSICAL, kernelPageCount);

    // Calculate space that is lost due to page alignment.
    u64 deadSpace{0};
    deadSpace += (u64)&DATA_START - (u64)&TEXT_END;
    deadSpace += (u64)&READ_ONLY_DATA_START - (u64)&DATA_END;
    deadSpace += (u64)&BLOCK_STARTING_SYMBOLS_START - (u64)&READ_ONLY_DATA_END;
    u64 kernelSize = reinterpret_cast<u64>(&KERNEL_END)
                     - reinterpret_cast<u64>(&KERNEL_START);
    u64 textSize = reinterpret_cast<u64>(&TEXT_END)
                   - reinterpret_cast<u64>(&TEXT_START);
    u64 dataSize = reinterpret_cast<u64>(&DATA_END)
                   - reinterpret_cast<u64>(&DATA_START);
    u64 rodataSize = reinterpret_cast<u64>(&READ_ONLY_DATA_END)
                     - reinterpret_cast<u64>(&READ_ONLY_DATA_START);
    u64 bssSize = reinterpret_cast<u64>(&BLOCK_STARTING_SYMBOLS_END)
                  - reinterpret_cast<u64>(&BLOCK_STARTING_SYMBOLS_START);
    std::print(
        "\033[32m"
        "Physical memory initialized"
        "\033[0m\n"
        "  Physical memory mapped from {:#016x} thru {:#016x} ({}MiB)\n"
        "  Kernel loaded at {:#016x} ({}MiB)\n"
        "  Kernel mapped from {:#016x} thru {:#016x} ({}KiB)\n"
        "    .text:   {:#016x} thru {:#016x} ({} bytes)\n"
        "    .data:   {:#016x} thru {:#016x} ({} bytes)\n"
        "    .rodata: {:#016x} thru {:#016x} ({} bytes)\n"
        "    .bss:    {:#016x} thru {:#016x} ({} bytes)\n"
        "    Lost to page alignment: {} bytes\n"
        "\n",
        PHYSICAL_BASE,
        PHYSICAL_BASE + total_ram(),
        TO_MiB(total_ram()),
        u64(&KERNEL_PHYSICAL),
        TO_MiB(&KERNEL_PHYSICAL),
        u64(&KERNEL_START),
        u64(&KERNEL_END),
        TO_KiB(kernelSize),
        u64(&TEXT_START),
        u64(&TEXT_END),
        textSize,
        u64(&DATA_START),
        u64(&DATA_END),
        dataSize,
        u64(&READ_ONLY_DATA_START),
        u64(&READ_ONLY_DATA_END),
        rodataSize,
        u64(&BLOCK_STARTING_SYMBOLS_START),
        u64(&BLOCK_STARTING_SYMBOLS_END),
        bssSize,
        deadSpace);
}

void print_debug_kib() {
    std::print(
        "Memory Manager Debug Information:\n"
        "  Total Memory: {}KiB\n"
        "  Free Memory: {}KiB\n"
        "  Used Memory: {}KiB\n"
        "\n",
        TO_KiB(total_ram()),
        TO_KiB(free_ram()),
        TO_KiB(used_ram()));
}

void print_debug_mib() {
    std::print(
        "Memory Manager Debug Information:\n"
        "  Total Memory: {}MiB\n"
        "  Free Memory: {}MiB\n"
        "  Used Memory: {}MiB\n"
        "\n",
        TO_MiB(total_ram()),
        TO_MiB(free_ram()),
        TO_MiB(used_ram()));
}

void print_debug() {
    if (total_ram() > MiB(64))
        print_debug_mib();
    else
        print_debug_kib();
}
}  // namespace Memory
