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

#ifndef LENSOR_OS_HEAP_H
#define LENSOR_OS_HEAP_H

#include <integers.h>
#include <new>

#define HEAP_VIRTUAL_BASE 0xffffffffff000000
#define HEAP_INITIAL_PAGES 1

#define HEAP_BYTE_ALIGN 16

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
};

void init_heap();

// Enlarge the heap by a given number of bytes, aligned to next-highest page-aligned value.
void expand_heap(u64 numBytes);

void heap_print_debug();
void heap_print_debug_summed();

extern void* sHeapStart;
extern void* sHeapEnd;

#endif
