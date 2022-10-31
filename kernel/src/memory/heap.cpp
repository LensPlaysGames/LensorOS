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

#include <memory/heap.h>

#include <cstr.h>
#include <debug.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <string.h>

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


void* malloc(size_t numBytes) {
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

void heap_print_debug_starchart() {
    // One character per 64 bytes of heap.
    constexpr u8 characterGranularity = 64;
    u64 heapSize = (u64)(sHeapEnd) - (u64)(sHeapStart);
    u64 totalChars = heapSize / characterGranularity + 1;
    u8* out = new u8[totalChars];
    memset(out, 0, totalChars);
    u64 freeLeftover = 0;
    u64 usedLeftover = 0;
    u64 offset = 0;
    HeapSegmentHeader* it = (HeapSegmentHeader*)sHeapStart;
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
    String heap_visualization((const char*)out);
    dbgmsg_s("Heap (64b per char): ");
    dbgrainbow(heap_visualization, ShouldNewline::Yes);
    dbgmsg_s("\r\n");
}

void heap_print_debug() {
    // TODO: Interesting information, like average allocation
    //       size, number of malloc vs free calls, etc.
    u64 heapSize = (u64)(sHeapEnd) - (u64)(sHeapStart);
    dbgmsg("[Heap]: Debug information:\r\n"
           "  Size:   %ull\r\n"
           "  Start:  %x\r\n"
           "  End:    %x\r\n"
           "  Regions:\r\n"
           , heapSize, sHeapStart, sHeapEnd);
    u64 i = 0;
    u64 usedCount = 0;
    float usedSpaceEfficiency = 0.0f;
    HeapSegmentHeader* it = (HeapSegmentHeader*)sHeapStart;
    while (it) {
        float efficiency = (float)it->length / (float)(it->length + sizeof(HeapSegmentHeader));
        dbgmsg("    Region %ull:\r\n"
               "      Free:   %s\r\n"
               "      Length: %ull (%ull) %%f\r\n"
               "      Header Address:  %x\r\n"
               "      Payload Address: %x\r\n",
               i,
               to_string(it->free),
               it->length,
               it->length + sizeof(HeapSegmentHeader),
               100.0f * efficiency,
               it,
               (u64)(it) + sizeof(HeapSegmentHeader));
        if (!it->free) {
            usedSpaceEfficiency += efficiency;
            usedCount++;
        }
        ++i;
        it = it->next;
    };
    dbgmsg("\r\n");

    dbgmsg("Heap Metadata vs Payload ratio in used regions (lower is better): %%f\r\n\r\n",
           100.0f * (1.0f - usedSpaceEfficiency / (float)usedCount));

    heap_print_debug_starchart();
}

void heap_print_debug_summed() {
    // TODO: Interesting information, like average allocation
    //       size, number of malloc vs free calls, etc.
    u64 heapSize = (u64)(sHeapEnd) - (u64)(sHeapStart);
    dbgmsg("[Heap]: Debug information:\r\n"
           "  Size:   %ull\r\n"
           "  Start:  %x\r\n"
           "  End:    %x\r\n"
           "  Regions:\r\n"
           , heapSize, sHeapStart, sHeapEnd);
    float usedSpaceEfficiency = 0.0f;
    u64 i = 0;
    u64 usedCount = 0;
    HeapSegmentHeader* it = (HeapSegmentHeader*)sHeapStart;
    while (it) {
        HeapSegmentHeader* start_it = it;
        u64 payload_total = it->length;
        u64 total_length = it->length + sizeof(HeapSegmentHeader);
        float efficiency = (float)it->length / (float)(it->length + sizeof(HeapSegmentHeader));
        if (!it->free) {
            usedSpaceEfficiency += efficiency;
            ++usedCount;
        }
        u64 start_i = i;
        bool free = it->free;
        while ((it = it->next)) {
            ++i;
            if (it->free != free) {
                break;
            }
            payload_total += it->length;
            total_length += it->length + sizeof(HeapSegmentHeader);
            float localEfficiency = (float)it->length / (float)(it->length + sizeof(HeapSegmentHeader));
            efficiency += localEfficiency;
            if (!it->free) {
                usedSpaceEfficiency += localEfficiency;
                ++usedCount;
            }
        }
        if (i - start_i == 1 || i - start_i == 0) {
            dbgmsg("    Region %ull:\r\n", start_i);
        } else {
            dbgmsg("    Region %ull through %ull:\r\n", start_i, i - 1);
        }
        efficiency = efficiency / (i - start_i ? i - start_i : 1);
        dbgmsg("      Free:          %s\r\n"
               "      Length:        %ull (%ull) %%f\r\n"
               "      Start Address: %x\r\n",
               to_string(free),
               payload_total, total_length, 100.0f * efficiency,
               start_it);
    };
    dbgmsg("\r\n");

    dbgmsg("Heap Metadata vs Payload ratio in used regions (lower is better): %%f\r\n\r\n",
           100.0f * (1.0f - (usedSpaceEfficiency / (float)usedCount)));

    heap_print_debug_starchart();
}

[[nodiscard]] void* operator new(size_t size) { return malloc(size); }
[[nodiscard]] void* operator new[](size_t size) { return malloc(size); }
void operator delete(void* ptr) noexcept { free(ptr); }
void operator delete[](void* ptr) noexcept { free(ptr); }
void operator delete(void* ptr, size_t) noexcept { free(ptr); }
void operator delete[](void* ptr, size_t) noexcept { free(ptr); }