#include "page_frame_allocator.h"

#include "../cstr.h"
#include "../efi_memory.h"
#include "../memory.h"

// Define global allocator for use anywhere within the kernel.
PageFrameAllocator gAlloc;

// Total amount of memory on the system.
u64 total_memory {0};
// Reserved by OS/hardware.
u64 reserved_memory {0};
// Available memory to be used when needed.
u64 free_memory {0};
// Memory that is currently in use.
u64 used_memory {0};

bool initialized = false;

void PageFrameAllocator::read_efi_memory_map(EFI_MEMORY_DESCRIPTOR* map, u64 mapSize, u64 mapDescSize) {
    if (initialized)
        return;
    u64 mapEntries = mapSize / mapDescSize;
    void* largestFreeMemorySegment = NULL;
    u64 largestFreeMemorySegmentSize = 0;
    for (u64 i = 0; i < mapEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)map + (i * mapDescSize));
        // 7 = EfiConventionalMemory (usable)
        if (desc->type == 7) {
            u64 sz = desc->numPages * 4096;
            if (sz > largestFreeMemorySegmentSize) {
                largestFreeMemorySegment = desc->physicalAddress;
                largestFreeMemorySegmentSize = sz;
            }
        }
    }
    u64 memorySize = get_memory_size(map, mapEntries, mapDescSize);
    total_memory = memorySize;
    free_memory = memorySize;
    u64 bitmapSize = memorySize / 4096 / 8 + 1;
    initialize_bitmap(bitmapSize, largestFreeMemorySegment);
    reserve_pages(0, memorySize / 4096 + 1);
    for (u64 i = 0; i < mapEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)map + (i * mapDescSize));
        if (desc->type == 7)
            unreserve_pages(desc->physicalAddress, desc->numPages);
    }
    lock_pages(PageBitmap.Buffer, PageBitmap.Size / 4096 + 1);
    initialized = true;
}

void PageFrameAllocator::initialize_bitmap(u64 bitmapSize, void* bufAddress) {
    PageBitmap.Size = bitmapSize;
    PageBitmap.Buffer = (u8*)bufAddress;
    for (u64 i = 0; i < PageBitmap.Size; ++i)
        *(u8*)(PageBitmap.Buffer + i) = 0;
}

// Find first empty page, lock it, return address.
// This number points to the page bitmap index of which there are no zeros behind it.
// This means this is the minimum index at which a new page OR set of pages may be found.
u64 gPageBitmapIndex = 0;
void* PageFrameAllocator::request_page() {
    for (; gPageBitmapIndex < PageBitmap.Size; gPageBitmapIndex++) {
        if (PageBitmap[gPageBitmapIndex] == true)
            continue;
        lock_page((void*)(gPageBitmapIndex * 4096));
        return (void*)(gPageBitmapIndex * 4096);
    }
    // TODO: Page frame swap to file
    return NULL;
}

// Find a run of `numPages` free pages, lock them, and return the address of the beginning of the run.
// ie.                   mem: 11110010000100011100
// RequestPages(3) would return here ^
void* PageFrameAllocator::request_pages(u64 numPages) {
    for (u64 i = gPageBitmapIndex; i < PageBitmap.Size; ++i) {
        if (PageBitmap[i] == true)
            continue;
        u64 index = i;
        u64 run = 0;
        while (PageBitmap[index] == false) {
            run++;
            index++;
            // TODO:
            // No memory matching criteria, should probably do a page frame swap from disk.
            if (index > PageBitmap.Size)
                return nullptr;
            if (run >= numPages) {
                void* out = (void*)(i * 4096);
                // LOCK PAGES
                lock_pages(out, numPages);
                // RETURN ADDRESS AT BEGINNING OF RUN
                return out;
            }
        }
        // If this point is reached, it means run was not long enough.
        // Start searching for next run after the run we've already determined is not long enough.
        i = index;
    }
    return nullptr;
}

void PageFrameAllocator::free_page(void* addr) {
    u64 index = (u64)addr / 4096;
    // Check if page is already free.
    if (PageBitmap[index] == false)
        return;
    if (PageBitmap.Set(index, false)) {
        free_memory += 4096;
        used_memory -= 4096;
        if (gPageBitmapIndex > index)
            gPageBitmapIndex = index;
    }
}

void PageFrameAllocator::free_pages(void* addr, u64 numPages) {
    for (u64 i = 0; i < numPages; ++i) {
        free_page((void*)((u64)addr + (i * 4096)));
    }
}

void PageFrameAllocator::lock_page(void* addr) {
    u64 index = (u64)addr / 4096;
    if (PageBitmap[index] == true)
        return;
    if (PageBitmap.Set(index, true)) {
        free_memory -= 4096;
        used_memory += 4096;
    }
}

void PageFrameAllocator::lock_pages(void* addr, u64 numPages) {
    for (u64 i = 0; i < numPages; ++i) {
        lock_page((void*)((u64)addr + (i * 4096)));
    }
}

void PageFrameAllocator::unreserve_page(void* addr) {
    u64 index = (u64)addr / 4096;
    // Check if page is already free.
    if (PageBitmap[index] == false)
        return;
    if (PageBitmap.Set(index, false)) {
        free_memory += 4096;
        reserved_memory -= 4096;
        if (gPageBitmapIndex > index)
            gPageBitmapIndex = index;
    }
}

void PageFrameAllocator::unreserve_pages(void* addr, u64 numPages) {
    for (u64 i = 0; i < numPages; ++i) {
        unreserve_page((void*)((u64)addr + (i * 4096)));
    }
}

void PageFrameAllocator::reserve_page(void* addr) {
    u64 index = (u64)addr / 4096;
    // Check if page is already locked.
    if (PageBitmap[index] == true)
        return;
    if (PageBitmap.Set(index, true)) {
        free_memory -= 4096;
        reserved_memory += 4096;
    }
}

void PageFrameAllocator::reserve_pages(void* addr, u64 numPages) {
    for (u64 i = 0; i < numPages; ++i) {
        reserve_page((void*)((u64)addr + (i * 4096)));
    }
}

u64 PageFrameAllocator::get_total_ram()    { return total_memory;    }
u64 PageFrameAllocator::get_free_ram()     { return free_memory;     }
u64 PageFrameAllocator::get_used_ram()     { return used_memory;     }
u64 PageFrameAllocator::get_reserved_ram() { return reserved_memory; }
