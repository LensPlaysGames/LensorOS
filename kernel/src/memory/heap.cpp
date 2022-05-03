#include <memory/heap.h>

#include <cstr.h>
#include <debug.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>

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
    if (last == nullptr)
        return;
    
    if (last->free == false)
        return;

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
    for (u64 i = 0; i < HEAP_INITIAL_PAGES * PAGE_SIZE; i += PAGE_SIZE) {
        // Map virtual heap position to physical memory address returned by page frame allocator.
        // FIXME: Should this be global?
        Memory::map((void*)((u64)HEAP_VIRTUAL_BASE + i), Memory::request_page()
                    , (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    | (u64)Memory::PageTableFlag::Global
                    );
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
    dbgmsg("[Heap]: \033[32mInitialized\033[0m\r\n"
           "  Virtual Address: %x thru %x\r\n"
           "  Size: %ull\r\n"
           "\r\n"
           , sHeapStart, sHeapEnd
           , numBytes);
    heap_print_debug();
}

void expand_heap(u64 numBytes) {
#ifdef DEBUG_HEAP
    dbgmsg("[Heap]: Expanding by %ull bytes\r\n"
           , numBytes);
#endif /* DEBUG_HEAP */
    u64 numPages = (numBytes / PAGE_SIZE) + 1;
    // Round byte count to page-aligned boundary.
    numBytes = numPages * PAGE_SIZE;
    // Get address of new header at the end of the heap.
    HeapSegmentHeader* extension = (HeapSegmentHeader*)sHeapEnd;
    // Allocate and map a page in memory for new header.
    for (u64 i = 0; i < numPages; ++i) {
        Memory::map(sHeapEnd, Memory::request_page()
                    , (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    | (u64)Memory::PageTableFlag::Global
                    );
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
    dbgmsg("[Heap]: malloc() -- numBytes=%ull\r\n"
           , numBytes);
#endif /* DEBUG_HEAP */
    // Start looking for a free segment at the start of the heap.
    HeapSegmentHeader* current = (HeapSegmentHeader*)sHeapStart;
    while (true) {
        if (current->free) {
            if (current->length > numBytes) {
                if (HeapSegmentHeader* split = current->split(numBytes)) {
                    split->free = false;
#ifdef DEBUG_HEAP
                    dbgmsg("  Made split.\r\n");
                    heap_print_debug();
#endif /* DEBUG_HEAP */
                    return (void*)((u64)split + sizeof(HeapSegmentHeader));
                }
            }
            else if (current->length == numBytes) {
                current->free = false;
#ifdef DEBUG_HEAP
                dbgmsg("  Found exact match.\r\n");
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
    dbgmsg("[Heap]: free() -- address=%x, numBytes=%ull\r\n"
           , address, segment->length);
#endif /* DEBUG_HEAP */
    segment->free = true;
    segment->combine_forward();
    segment->combine_backward();
#ifdef DEBUG_HEAP
    heap_print_debug();
#endif /* DEBUG_HEAP */
}

void heap_print_debug() {
    // TODO: Interesting information, like average allocation
    //       size, number of malloc vs free calls, etc.
    u64 heapSize = reinterpret_cast<u64>(sHeapEnd)
        - reinterpret_cast<u64>(sHeapStart);
    dbgmsg("[Heap]: Debug information:\r\n"
           "  Size:   %ull\r\n"
           "  Start:  %x\r\n"
           "  End:    %x\r\n"
           "  Regions:\r\n"
           , heapSize, sHeapStart, sHeapEnd);
    u64 i = 0;
    HeapSegmentHeader* it = (HeapSegmentHeader*)sHeapStart;
    do {
        dbgmsg("    Region %ull:\r\n"
               "      Free:   %s\r\n"
               "      Length: %ull (%ull)\r\n"
               "      Header Address:  %x\r\n"
               "      Payload Address: %x\r\n"
               , i
               , to_string(it->free)
               , it->length
               , it->length + sizeof(HeapSegmentHeader)
               , it
               , reinterpret_cast<u64>(it) + sizeof(HeapSegmentHeader)
               );
        ++i;
        it = it->next;
    } while (it);
    dbgmsg("\r\n");

    // One character per 64 bytes of heap.
    constexpr u8 characterGranularity = 64;
    u64 totalChars = heapSize / characterGranularity + 1;
    u8* out = new u8[totalChars];
    memset(out, 0, totalChars);
    u64 freeLeftover = 0;
    u64 usedLeftover = 0;
    u64 offset = 0;
    it = (HeapSegmentHeader*)sHeapStart;
    do {
        if (offset >= totalChars)
            break;

        u64 segmentSize = it->length + sizeof(HeapSegmentHeader);
        u64 delta = segmentSize % characterGranularity;
        u64 numChars = segmentSize / characterGranularity;
        if (it->free) {
            freeLeftover += delta;
            if (freeLeftover > characterGranularity) {
                out[offset] = '_';
                offset++;
                freeLeftover -= characterGranularity;
            }
        }
        else {
            usedLeftover += delta;
            if (usedLeftover > characterGranularity) {
                out[offset] = '*';
                offset++;
                usedLeftover -= characterGranularity;
            }
        }
        char c = it->free ? '_' : '*';
        memset(&out[offset], c, numChars);
        offset += numChars;
        it = it->next;
    } while (it != nullptr);

    dbgmsg("Heap (64b per char): %s\r\n", out);
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
