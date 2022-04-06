#include "heap.h"

#include "../cstr.h"
#include "common.h"
#include "physical_memory_manager.h"
#include "virtual_memory_manager.h"
#include "../uart.h"

// Uncomment the following directive for extra debug information output.
//#define DEBUG_HEAP

void* sHeapStart { nullptr };
void* sHeapEnd { nullptr };
HeapSegmentHeader* sLastHeader { nullptr };

void HeapSegmentHeader::combine_forward() {
    // Can't combine nothing :^).
    if (next == nullptr)
        return;
    // Don't combine a header that is in use.
    if (next->free == false)
        return;
    // Update last header address if it is being changed.
    if (next == sLastHeader)
        sLastHeader = this;
    // Set next next segment last to this segment.
    if (next->next != nullptr)
        next->next->last = this;
    
    length = length + next->length + sizeof(HeapSegmentHeader);
    next = next->next;
}

void HeapSegmentHeader::combine_backward() {
    if (last != nullptr && last->free)
        last->combine_forward();
}

HeapSegmentHeader* HeapSegmentHeader::split(u64 splitLength) {
    if (splitLength + sizeof(HeapSegmentHeader) > length)
        return nullptr;

    if (splitLength < 8)
        return nullptr;
    
    /// Length of segment that is leftover after creating new header of `splitLength` length.
    u64 splitSegmentLength = length - splitLength - sizeof(HeapSegmentHeader);
    if (splitSegmentLength < 8)
        return nullptr;

    /// Position of header that is newly created within the middle of `this` header.
    HeapSegmentHeader* splitHeader = (HeapSegmentHeader*)((u64)this + sizeof(HeapSegmentHeader) + splitLength);

    //HeapSegmentHeader* splitHeader = (HeapSegmentHeader*)((u64)this + splitSegmentLength);
    if (next) {
        // Set next segment's last segment to the new segment.
        next->last = splitHeader;
        // Set new segment's next segment.
        splitHeader->next = next;       
    }
    else {
        splitHeader->next = nullptr;
        sLastHeader = splitHeader;
    }
    // Set current segment next to newly inserted segment.
    next = splitHeader;
    // Set new segment's last segment to this segment.
    splitHeader->last = this;
    // Set new length of segments.
    length = splitLength;
    splitHeader->length = splitSegmentLength;
    splitHeader->free = free;
    return this;
}

void init_heap() {
    u64 numBytes = HEAP_INITIAL_PAGES * PAGE_SIZE;
    for (u64 i = 0; i < HEAP_INITIAL_PAGES; ++i) {
        // Map virtual heap position to physical memory address returned by page frame allocator.
        Memory::map((void*)((u64)HEAP_VIRTUAL_BASE + (i * PAGE_SIZE)), Memory::request_page());
    }
    sHeapStart = (void*)HEAP_VIRTUAL_BASE;
    sHeapEnd = (void*)((u64)sHeapStart + numBytes);
    HeapSegmentHeader* firstSegment = (HeapSegmentHeader*)HEAP_VIRTUAL_BASE;
    // Actual length of free memory has to take into account header.
    firstSegment->length = numBytes - sizeof(HeapSegmentHeader);
    firstSegment->next = nullptr;
    firstSegment->last = nullptr;
    firstSegment->free = true;
    sLastHeader = firstSegment;

    UART::out("[Heap]: \033[32mInitialized\033[0m\r\n  Virtual Address: 0x");
    UART::out(to_hexstring<void*>(sHeapStart));
    UART::out(" thru 0x");
    UART::out(to_hexstring<void*>(sHeapEnd));
    UART::out("\r\n  Size: ");
    UART::out(numBytes);
    UART::out("\r\n\r\n");
    heap_print_debug();
}

void expand_heap(u64 numBytes) {
#ifdef DEBUG_HEAP
    UART::out("[Heap]: Expanding by ");
    UART::out(numBytes);
    UART::out(" bytes\r\n");
#endif /* DEBUG_HEAP */
    u64 numPages = (numBytes / PAGE_SIZE) + 1;
    // Round byte count to page-aligned boundary.
    numBytes = numPages * PAGE_SIZE;
    // Get address of new header at the end of the heap.
    HeapSegmentHeader* extension = (HeapSegmentHeader*)sHeapEnd;
    // Allocate and map a page in memory for new header.
    for (u64 i = 0; i < numPages; ++i) {
        Memory::map(sHeapEnd, Memory::request_page());
        sHeapEnd = (void*)((u64)sHeapEnd + PAGE_SIZE);
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


void* malloc(u64 numBytes) {
    // Can not allocate nothing.
    if (numBytes == 0)
        return nullptr;
    // Round numBytes to 64-bit (8-byte) aligned number.
    if (numBytes % 8 > 0) {
        numBytes -= (numBytes % 8);
        numBytes += 8;
    }
#ifdef DEBUG_HEAP
    UART::out("[Heap]: malloc() -- numBytes=");
    UART::out(numBytes);
    UART::out("\r\n");
#endif /* DEBUG_HEAP */
    // Start looking for a free segment at the start of the heap.
    HeapSegmentHeader* current = (HeapSegmentHeader*)sHeapStart;
    while (true) {
        if (current->free) {
            if (current->length > numBytes) {
                if (HeapSegmentHeader* split = current->split(numBytes)) {
                    split->free = false;
#ifdef DEBUG_HEAP
                    UART::out("  Made split.\r\n");
                    heap_print_debug();
#endif /* DEBUG_HEAP */
                    return (void*)((u64)split + sizeof(HeapSegmentHeader));
                }
            }
            else if (current->length == numBytes) {
                current->free = false;
#ifdef DEBUG_HEAP
                UART::out("  Found exact match.\r\n");
                heap_print_debug();
#endif /* DEBUG_HEAP */
                return (void*)((u64)current + sizeof(HeapSegmentHeader));
            }
        }
        if (current->next == nullptr)
            break;
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
    HeapSegmentHeader* segment = (HeapSegmentHeader*)((u64)address - sizeof(HeapSegmentHeader));
#ifdef DEBUG_HEAP
    u64 len = segment->length;
#endif /* DEBUG_HEAP */
    segment->free = true;
    segment->combine_forward();
    segment->combine_backward();
#ifdef DEBUG_HEAP
    UART::out("[Heap]: free() -- numBytes=");
    UART::out(len);
    UART::out("\r\n");
    heap_print_debug();
#endif /* DEBUG_HEAP */
}

void heap_print_debug() {
    UART::out("[Heap]: Debug information dump:\r\n  Size: ");
    UART::out(TO_KiB((u64)sHeapEnd - (u64)sHeapStart));
    UART::out("KiB\r\n  Start: 0x");
    UART::out(to_hexstring(sHeapStart));
    UART::out("\r\n  End: 0x");
    UART::out(to_hexstring(sHeapEnd));
    UART::out("\r\n  Regions:");
    u64 i = 0;
    HeapSegmentHeader* it = (HeapSegmentHeader*)sHeapStart;
    do {
        UART::out("\r\n    Region ");
        UART::out(i);
        UART::out(":\r\n      Free: ");
        UART::out(to_string(it->free));
        UART::out("\r\n      Address: 0x");
        UART::out(to_hexstring<HeapSegmentHeader*>(it));
        UART::out("\r\n      Length: ");
        UART::out(it->length);
        ++i;
        it = it->next;
    } while (it);
    UART::out("\r\n\r\n");
}

void* operator new   (u64 numBytes) { return malloc(numBytes); }
void* operator new[] (u64 numBytes) { return malloc(numBytes); }
void  operator delete   (void* address) noexcept { return free(address); }
void  operator delete[] (void* address) noexcept { return free(address); }

void operator delete (void* address, u64 unused) {
  (void)unused;
  return free(address);
}
void operator delete[] (void* address, u64 unused) {
  (void)unused;
  return free(address);
}
