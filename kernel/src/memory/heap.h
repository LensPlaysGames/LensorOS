#ifndef LENSOR_OS_HEAP_H
#define LENSOR_OS_HEAP_H

#include "../integers.h"

// TODO: Store physical address (or make it easy to
//   convert between physical/virtual addresses).
struct HeapSegmentHeader {
    // Doubly linked list
    HeapSegmentHeader* last { nullptr };
    HeapSegmentHeader* next { nullptr };
    // Data fields
    u64 length { 0 };
    bool free { false };
    // Fragmentation Prevention
    void combine_forward();
    void combine_backward();
    // Allocation
    HeapSegmentHeader* split(u64 splitLength);
} __attribute__((packed));

// INITIALIZATION
void init_heap(void* startAddress, u64 numInitialPages);

// Enlarge the heap by a given number of bytes, aligned to next-highest page-aligned value.
void expand_heap(u64 numBytes);

// PUBLIC API
void* malloc(u64 numBytes);
void free(void* address);

void* operator new (u64 numBytes);
void* operator new[] (u64 numBytes);
void operator delete (void* address) noexcept;
void operator delete[] (void* address) noexcept;

/// According to www.cplusplus.com on the C++14 standard, delete calls
///   with an extra `unsigned long` parameter just calls delete.
void operator delete (void* address, u64 unused);
void operator delete[] (void* address, u64 unused);

void heap_print_debug();

extern void* sHeapStart;
extern void* sHeapEnd;

#endif