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
#include <format>
#include <memory.h>
#include <memory/common.h>
#include <memory/paging.h>
#include <memory/physical_memory_manager.h>
#include <memory/virtual_memory_manager.h>
#include <scheduler.h>
#include <string>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_HEAP

#ifdef DEBUG_HEAP
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...)
#endif

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

    if (splitLength < HEAP_BYTE_ALIGN)
        return nullptr;

    /// Length of segment that is leftover after creating new header of `splitLength` length.
    u64 splitSegmentLength = length - splitLength - sizeof(HeapSegmentHeader);
    if (splitSegmentLength < HEAP_BYTE_ALIGN)
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
    // NOTE: We don't use map_pages here because we request a new page for each one mapped.
    for (u64 i = 0; i < HEAP_INITIAL_PAGES * PAGE_SIZE; i += PAGE_SIZE) {
        // Map virtual heap position to physical memory address returned by page frame allocator.
        // FIXME: Should this be global?
        Memory::map((void*)((u64)HEAP_VIRTUAL_BASE + i), Memory::request_page()
                    , (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    //| (u64)Memory::PageTableFlag::Global
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
    std::print("[Heap]: \033[32mInitialized\033[0m\n"
               "  Virtual Address: {} thru {}\n"
               "  Size: {}\n"
               "\n"
               , sHeapStart, sHeapEnd
               , numBytes);
    heap_print_debug();
}

void expand_heap(u64 numBytes) {
    // Get page count (at least one) from number of bytes
    u64 numPages = (numBytes / PAGE_SIZE) + 1;
    // Round byte count to page-aligned boundary.
    numBytes = numPages * PAGE_SIZE;

    DBGMSG("[Heap]: Expanding by {} bytes\n" , numBytes);

    // NOTE: We don't use map_pages here because we request a new page for each one mapped.
    for (u64 i = 0; i < numPages * PAGE_SIZE; i += PAGE_SIZE) {
        // Map virtual heap position to physical memory address returned by page frame allocator.
        void* addr = Memory::request_page();
        memset(addr, 0, PAGE_SIZE);
        Scheduler::map_pages_in_all_processes
            ((void*)((u64)sHeapEnd + i), addr
             , (u64)Memory::PageTableFlag::Present
             | (u64)Memory::PageTableFlag::ReadWrite
             , 1
             );
        Memory::map(Memory::active_page_map()
                    , (void*)((u64)sHeapEnd + i), addr
                    , (u64)Memory::PageTableFlag::Present
                    | (u64)Memory::PageTableFlag::ReadWrite
                    , Memory::ShowDebug::No
                    );

        std::print("Mapped {} to {}\n", (void*)((u64)sHeapEnd + i), addr);
    }

    // Get address of new header at the end of the heap.
    auto* extension = (HeapSegmentHeader*)sHeapEnd;
    DBGMSG("  extension begin addr={}\n", (void*)extension);

    sHeapEnd = (void*)((u64)extension + numBytes);
    DBGMSG("  extension end addr={}\n", sHeapEnd);

    extension->free = true;
    extension->last = sLastHeader;
    sLastHeader->next = extension;
    sLastHeader = extension;
    extension->next = nullptr;
    extension->length = numBytes - sizeof(HeapSegmentHeader);

    // After expanding, combine with the previous segment (decrease fragmentation).
    extension->combine_backward();
    DBGMSG("  \033[32mHeap expansion successful\033[0m\n");
}


void* malloc(size_t numBytes) {
    // Can not allocate nothing.
    if (numBytes == 0)
        return nullptr;
    // Round numBytes to HEAP_BYTE_ALIGN aligned number.
    if (numBytes % HEAP_BYTE_ALIGN > 0) {
        numBytes -= (numBytes % HEAP_BYTE_ALIGN);
        numBytes += HEAP_BYTE_ALIGN;
    }
    DBGMSG("[Heap]: malloc() -- numBytes={}\n", numBytes);
    // Start looking for a free segment at the start of the heap.
    auto* current = (HeapSegmentHeader*)sHeapStart;
    while (true) {
        if (current->free) {
            if (current->length > numBytes) {
                if (HeapSegmentHeader* split = current->split(numBytes)) {
                    split->free = false;
                    DBGMSG("  Made split.\n");
                    return (void*)((usz)split + sizeof(HeapSegmentHeader));
                }
            }
            else if (current->length == numBytes) {
                current->free = false;
                DBGMSG("  Found exact match.\n");
                return (void*)((usz)current + sizeof(HeapSegmentHeader));
            }
        }
        if (current->next == nullptr) break;
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
    if (((usz)address & HEAP_VIRTUAL_BASE) != HEAP_VIRTUAL_BASE) {
        DBGMSG("[Heap]: free() -- Denying free of address {} as it does not look like a heap pointer\n", address);
        return;
    }
    auto* segment = (HeapSegmentHeader*)((usz)address - sizeof(HeapSegmentHeader));
    [[maybe_unused]]u64 length = segment->length;
    DBGMSG("[Heap]: free() -- address={}, numBytes={}\n", address, length);
    segment->free = true;
    segment->combine_forward();
    segment->combine_backward();
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
    auto* it = (HeapSegmentHeader*)sHeapStart;
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
    std::print("Heap (64b per char): ");
    dbgrainbow(__s(out), ShouldNewline::Yes);
    std::print("\n");
}

void heap_print_debug() {
    // TODO: Interesting information, like average allocation
    //       size, number of malloc vs free calls, etc.
    u64 heapSize = (u64)(sHeapEnd) - (u64)(sHeapStart);
    std::print("[Heap]: Debug information:\n"
               "  Size:   {}\n"
               "  Start:  {}\n"
               "  End:    {}\n"
               "  Regions:\n"
               , heapSize, sHeapStart, sHeapEnd);
    u64 i = 0;
    u64 usedCount = 0;
    auto* it = (HeapSegmentHeader*)sHeapStart;
    while (it) {
        std::print("    Region {}:\n"
                   "      Free:   {}\n"
                   "      Length: {} ({})\n"
                   "      Header Address:  {}\n"
                   "      Payload Address: {}\n"
                   , i
                   , it->free
                   , u64(it->length)
                   , it->length + sizeof(HeapSegmentHeader)
                   , (void*) it
                   , (void*)(u64(it) + sizeof(HeapSegmentHeader)));
        if (!it->free) usedCount++;
        ++i;
        it = it->next;
    };

    heap_print_debug_starchart();
}

void heap_print_debug_summed() {
    // TODO: Interesting information, like average allocation
    //       size, number of malloc vs free calls, etc.
    u64 heapSize = (u64)(sHeapEnd) - (u64)(sHeapStart);
    std::print("[Heap]: Debug information:\n"
               "  Size:   {}\n"
               "  Start:  {}\n"
               "  End:    {}\n"
               "  Regions:\n"
               , heapSize, sHeapStart, sHeapEnd);
    u64 i = 0;
    u64 usedCount = 0;
    auto* it = (HeapSegmentHeader*)sHeapStart;
    while (it) {
        auto* start_it = it;
        u64 payload_total = it->length;
        u64 total_length = it->length + sizeof(HeapSegmentHeader);
        if (!it->free) ++usedCount;
        u64 start_i = i;
        bool free = it->free;
        while ((it = it->next)) {
            ++i;
            if (it->free != free) break;
            payload_total += it->length;
            total_length += it->length + sizeof(HeapSegmentHeader);
            if (!it->free) ++usedCount;
        }
        if (i - start_i == 1 || i - start_i == 0)
            std::print("    Region {}:\n", start_i);
        else std::print("    Region {} through {}:\n", start_i, i - 1);
        std::print("      Free:          {}\n"
                   "      Length:        {} ({})\n"
                   "      Start Address: {}\n",
                   free,
                   payload_total, total_length,
                   (void*) start_it);
    };

    heap_print_debug_starchart();
}

[[nodiscard]] void* operator new(size_t size) { return malloc(size); }
[[nodiscard]] void* operator new[](size_t size) { return malloc(size); }
void operator delete(void* ptr) noexcept { free(ptr); }
void operator delete[](void* ptr) noexcept { free(ptr); }
void operator delete(void* ptr, size_t) noexcept { free(ptr); }
void operator delete[](void* ptr, size_t) noexcept { free(ptr); }

[[nodiscard]] void* operator new(size_t, void* ptr) noexcept { return ptr; }
[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept { return ptr; }
