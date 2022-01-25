#include "heap.h"

void* sHeapStart;
void* sHeapEnd;
HeapSegmentHeader* sLastHeader;

void HeapSegmentHeader::combine_forward() {
	if (next == nullptr)     { return; }
	if (next->free == false) { return; }
	if (next == sLastHeader) { sLastHeader = this; }
	// Set next next segment last to this segment.
	if (next->next != nullptr) {
		next->next->last = this;
	}
	length = length + next->length + sizeof(HeapSegmentHeader);
	next = next->next;
}

void HeapSegmentHeader::combine_backward() {
	if (last != nullptr && last->free) {
		last->combine_forward();
	}
}

HeapSegmentHeader* HeapSegmentHeader::split(uint64_t splitLength) {
	if (splitLength < 8) { return nullptr; }
	
	/// Length of segment that is leftover after creating new header of `splitLength` length.
	uint64_t splitSegmentLength = length - splitLength - sizeof(HeapSegmentHeader);
	if (splitSegmentLength < 8) { return nullptr; }

	/// Position of header that is newly created within the middle of `this` header.
	HeapSegmentHeader* splitHeader = (HeapSegmentHeader*)((uint64_t)this
														  + sizeof(HeapSegmentHeader)
														  + splitLength);
	if (next != nullptr) {
		// Set next segment's last segment to the new segment.
		next->last = splitHeader;
		// Set new segment's next segment.
		splitHeader->next = next;
	}
	// Set current segment next to newly inserted segment.
	next = splitHeader;
	// Set new segment's last segment to this segment.
	splitHeader->last = this;
	// Update values to reflect split.
	splitHeader->length = splitSegmentLength;
	splitHeader->free = free;
	length = splitLength;
	if (sLastHeader == this) {
		sLastHeader = splitHeader;
	}
	return splitHeader;
}

void init_heap(void* startAddress, uint64_t numInitialPages) {
	for (uint64_t i = 0; i < numInitialPages; ++i) {
		// Map virtual heap position to physical memory address returned by page frame allocator.
		gPTM.map_memory((void*)((uint64_t)startAddress + (i * 0x1000)), gAlloc.request_page());
	}
	// Start of heap.
	sHeapStart = startAddress;
	// End of heap.
	uint64_t numBytes = numInitialPages * 0x1000;
	sHeapEnd = (void*)((uint64_t)sHeapStart + numBytes);
	HeapSegmentHeader* firstSegment = (HeapSegmentHeader*)startAddress;
	// Actual length of free memory has to take into account header.
	firstSegment->length = numBytes - sizeof(HeapSegmentHeader);
	firstSegment->next = nullptr;
	firstSegment->last = nullptr;
	firstSegment->free = true;
	sLastHeader = firstSegment;
}

void expand_heap(uint64_t numBytes) {
	uint64_t numPages = (numBytes / 0x1000) + 1;
	// Get address of new header at the end of the heap.
	HeapSegmentHeader* extension = (HeapSegmentHeader*)sHeapEnd;
	// Allocate and map a page in memory for new header.
	for (uint64_t i = 0; i < numPages; ++i) {
		gPTM.map_memory(sHeapEnd, gAlloc.request_page());
		sHeapEnd = (void*)((uint64_t)sHeapEnd + 0x1000);
	}
	extension->free = true;
	extension->last = sLastHeader;
	sLastHeader->next = extension;
	sLastHeader = extension;
	extension->next = nullptr;
	extension->length = numBytes - sizeof(HeapSegmentHeader);
	// After expanding, combine with the previous segment (decrease fragmentation).
	extension->combine_backward();
}

void* malloc(uint64_t numBytes) {
	// Can not allocate nothing.
	if (numBytes == 0) {
		return nullptr;
	}
	// Round numBytes to 64-bit (8-byte) aligned number.
	if (numBytes % 8 > 0) {
		numBytes -= (numBytes % 8);
		numBytes += 8;
	}

	HeapSegmentHeader* current = (HeapSegmentHeader*)sHeapStart;
	while (true) {
		if (current->free) {
			if (current->length > numBytes) {
				current->split(numBytes);
				current->free = false;
				return (void*)((uint64_t)current + sizeof(HeapSegmentHeader));
			}
			else if (current->length == numBytes) {
				current->free = false;
				return (void*)((uint64_t)current + sizeof(HeapSegmentHeader));
			}
		}
		if (current->next == nullptr) { break; }
		current = current->next;
	}
	// If this point is reached:
	// 1. No free segment of size has been found
	// 2. All existing segments have been searched
	// From here, we must allocate more memory for the heap (expand it),
	//   then do the same search once again. There is some optimization to be done here.
	expand_heap(numBytes);
	return malloc(numBytes);
}

void free(void* address) {
	HeapSegmentHeader* segment = (HeapSegmentHeader*)((uint64_t)address - sizeof(HeapSegmentHeader));
	segment->free = true;
	segment->combine_forward();
	segment->combine_backward();
}
