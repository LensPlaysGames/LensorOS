#include "page_frame_allocator.h"

// Define global allocator for use anywhere within the kernel.
PageFrameAllocator gAlloc;

// Reserved by OS/hardware.
uint64_t reserved_memory;
// Available memory to use.
uint64_t free_memory;
// Used by the user-space.
uint64_t used_memory;

bool initialized = false;

void PageFrameAllocator::ReadEfiMemoryMap(EFI_MEMORY_DESCRIPTOR* map, size_t mapSize, size_t mapDescSize) {
	if (initialized) { return; }
	uint64_t mapEntries = mapSize / mapDescSize;
	void* largestFreeMemorySegment = NULL;
	size_t largestFreeMemorySegmentSize = 0;
	for (int i = 0; i < mapEntries; i++) {
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)map + (i * mapDescSize));
		// 7 = EfiConventionalMemory (usable)
		if (desc->type == 7) {
			uint64_t sz = desc->numPages * 4096;
			if (sz > largestFreeMemorySegmentSize) {
				largestFreeMemorySegment = desc->physicalAddress;
				largestFreeMemorySegmentSize = sz;
			}
		}
	}
	uint64_t memorySize = GetMemorySize(map, mapEntries, mapDescSize);
	free_memory = memorySize;
	uint64_t bitmapSize = memorySize / 4096 / 8 + 1;
	InitializeBitmap(bitmapSize, largestFreeMemorySegment);
	LockPages(PageBitmap.Buffer, PageBitmap.Size / 4096 + 1);
	for (int i = 0; i < mapEntries; i++) {
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)map + (i * mapDescSize));
		if (desc->type != 7) {
			ReservePages(desc->physicalAddress, desc->numPages);
		}
	}
	initialized = true;
}

void PageFrameAllocator::InitializeBitmap(size_t bitmapSize, void* buf_address) {
	PageBitmap.Size = bitmapSize;
	PageBitmap.Buffer = (uint8_t*)buf_address;
    for (int i = 0; i < PageBitmap.Size; i++) {
		*(uint8_t*)(PageBitmap.Buffer + i) = 0;
	}
}

// Find first empty page, lock it, return address.
// This number points to the page bitmap index of which there are no zeros behind it.
// This means this is the minimum index at which a new page OR set of pages may be found.
uint64_t gPageBitmapIndex = 0;
void* PageFrameAllocator::RequestPage() {
	for (; gPageBitmapIndex < PageBitmap.Size; gPageBitmapIndex++) {
		if (PageBitmap[gPageBitmapIndex] == true) { continue; }
		LockPage((void*)(gPageBitmapIndex * 4096));
		return (void*)(gPageBitmapIndex * 4096);
	}
	// TODO: Page frame swap to file
	return NULL;
}

// Find a run of `numPages` free pages, lock them, and return the address of the beginning of the run.
// ie.                   mem: 11110010000100011100
// RequestPages(3) would return here ^
void* PageFrameAllocator::RequestPages(uint64_t numPages) {
	for (; gPageBitmapIndex < PageBitmap.Size; gPageBitmapIndex++) {
		if (PageBitmap[gPageBitmapIndex] == true) { continue; }
		uint64_t index_at_start = gPageBitmapIndex;
		uint64_t index = index_at_start;
		uint64_t run = 0;
		while (PageBitmap[index] == false) {
			run++;
			index++;
			if (index > PageBitmap.Size) {
				// TODO:
				// No memory matching criteria, should probably do a page frame swap from disk.
				// I don't have disk operations implemented yet, or even a file system, so maybe
				//   that's a task for another day.
				return NULL;
			}
			if (run == numPages) {
				void* out = (void*)(index_at_start * 0x1000);
				// LOCK PAGES
				LockPages(out, numPages);
				// RETURN ADDRESS AT BEGINNING OF RUN
				return out;
			}
		}
	}
	return NULL;
}

void PageFrameAllocator::FreePage(void* addr) {
	uint64_t index = (uint64_t)addr / 4096;
	// Check if page is already free.
	if (PageBitmap[index] == false) { return; }
	if (PageBitmap.Set(index, false)) {
		free_memory += 4096;
		used_memory -= 4096;
		if (gPageBitmapIndex > index) {
			gPageBitmapIndex = index;
		}
	}
}

void PageFrameAllocator::FreePages(void* addr, uint64_t numPages) {
    for (int i = 0; i < numPages; i++) {
	    FreePage((void*)((uint64_t)addr + (i * 4096)));
	}
}

void PageFrameAllocator::LockPage(void* addr) {
	uint64_t index = (uint64_t)addr / 4096;
	// Check if page is already locked.
	if (PageBitmap[index] == true) { return; }
	if (PageBitmap.Set(index, true)) {
		free_memory -= 4096;
		used_memory += 4096;
	}
}

void PageFrameAllocator::LockPages(void* addr, uint64_t numPages) {
    for (int i = 0; i < numPages; i++) {
	    LockPage((void*)((uint64_t)addr + (i * 4096)));
	}
}

void PageFrameAllocator::UnreservePage(void* addr) {
    uint64_t index = (uint64_t)addr / 4096;
	// Check if page is already free.
	if (PageBitmap[index] == false) { return; }
	if (PageBitmap.Set(index, false)) {
		free_memory += 4096;
		reserved_memory -= 4096;
		if (gPageBitmapIndex > index) {
			gPageBitmapIndex = index;
		}
	}
}

void PageFrameAllocator::UnreservePages(void* addr, uint64_t numPages) {
    for (int i = 0; i < numPages; i++) {
	    UnreservePage((void*)((uint64_t)addr + (i * 4096)));
	}
}

void PageFrameAllocator::ReservePage(void* addr) {
	uint64_t index = (uint64_t)addr / 4096;
	// Check if page is already locked.
	if (PageBitmap[index] == true) { return; }
	if (PageBitmap.Set(index, true)) {
		free_memory -= 4096;
		reserved_memory += 4096;
	}
}

void PageFrameAllocator::ReservePages(void* addr, uint64_t numPages) {
    for (int i = 0; i < numPages; i++) {
	    ReservePage((void*)((uint64_t)addr + (i * 4096)));
	}
}

uint64_t PageFrameAllocator::GetFreeRAM() { return free_memory; }
uint64_t PageFrameAllocator::GetUsedRAM() { return used_memory; }
uint64_t PageFrameAllocator::GetReservedRAM() { return reserved_memory; }
