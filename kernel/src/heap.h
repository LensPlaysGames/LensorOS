#ifndef LENSOR_OS_HEAP_H
#define LENSOR_OS_HEAP_H

#include "integers.h"
#include "paging/page_table_manager.h"

struct HeapSegmentHeader {
	// Doubly linked list
	HeapSegmentHeader* last;
	HeapSegmentHeader* next;
	// Data fields
	u64 length;
	bool free;
	// Fragmentation Prevention
	void combine_forward();
	void combine_backward();
	// Allocation
	HeapSegmentHeader* split(u64 splitLength);
};

// INITIALIZATION
void init_heap(void* startAddress, u64 numInitialPages);

// Enlarge the heap by a given number of bytes, aligned to next-highest page-aligned value.
void expand_heap(u64 numBytes);

// PUBLIC API
void* malloc(u64 numBytes);
void free(void* address);

inline void* operator new      (u64 numBytes)  { return malloc(numBytes); }
inline void* operator new[]    (u64 numBytes)  { return malloc(numBytes); }
inline void  operator delete   (void* address) { return free(address);    }
inline void  operator delete[] (void* address) { return free(address);    }

/// According to www.cplusplus.com on the C++14 standard, delete calls
///   with an extra `unsigned long` parameter just calls delete.
inline void operator delete (void* address, u64 unused) { return free(address); }
#endif
