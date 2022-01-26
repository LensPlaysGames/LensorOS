#ifndef LENSOR_OS_HEAP_H
#define LENSOR_OS_HEAP_H

#include <stdint.h>
#include "paging/page_table_manager.h"

struct HeapSegmentHeader {
	// Doubly linked list
	HeapSegmentHeader* last;
	HeapSegmentHeader* next;
	// Data fields
	uint64_t length;
	bool free;
	// Fragmentation Prevention
	void combine_forward();
	void combine_backward();
	// Allocation
	HeapSegmentHeader* split(uint64_t splitLength);
};

// INITIALIZATION
void init_heap(void* startAddress, uint64_t numInitialPages);

// Enlarge the heap by a given number of bytes, aligned to next-highest page-aligned value.
void expand_heap(uint64_t numBytes);

// PUBLIC API
void* malloc(uint64_t numBytes);
void free(void* address);

inline void* operator new      (uint64_t numBytes) { return malloc(numBytes); }
inline void* operator new[]    (uint64_t numBytes) { return malloc(numBytes); }
inline void  operator delete   (void* address)     { return free(address);    }
inline void  operator delete   (void* address, uint64_t ignore) { return free(address);    }
inline void  operator delete[] (void* address)     { return free(address);    }

#endif
