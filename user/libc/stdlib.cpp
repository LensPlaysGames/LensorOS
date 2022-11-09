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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>.
 */

#include "stdlib.h"

#include "assert.h"
#include "bits/cdtors.h"
#include "bits/file_struct.h"
#include "errno.h"
#include "stddef.h"
#include "stdio.h"
#include "string.h"
#include "sys/syscalls.h"
#include "unistd.h"

#include <algorithm>
#include <bits/abi.h>
#include <bits/colours.h>

/// ===========================================================================
///  STDLIB Implementation.
/// ===========================================================================
namespace {
/// ===========================================================================
///  Globals.
/// ===========================================================================
/// TODO: This MUST be thread-local.
int __errno { 0 };

/// Temporary workaround to allow dynamic memory allocation.
char heap_base[1'000'000];
char* heap_ptr;

/// Keep track of allocated blocks.
struct alignas (max_align_t) alloc_header {
    char* ptr;
    size_t size;
    alloc_header* next;
    alloc_header* prev;
};

/// Keep track of the last allocated block.
alloc_header* alloc_list;

/// Keep track of the last freed block.
alloc_header* free_list;

/// Free list for alloc_headers.
alloc_header* free_headers;

[[maybe_unused]] void dump_list(const char* tag, alloc_header* list) {
    __write(tag);
    __write_ptr(list);
    if (list) {
        list = list->next;
        while (list) {
            __write(" -> ");
            __write_ptr(list->ptr);
            list = list->next;
        }
    }
    __write("\n");
}

/// ===========================================================================
///  Malloc
/// ===========================================================================
/// Check if the heap has enough space for a new allocation.
bool heap_has_space(size_t size) {
    return heap_ptr + size < heap_base + sizeof(heap_base);
}

/// Allocate a new pointer on the heap.
__attribute__((malloc)) void* heap_alloc(size_t size) {
    if (!heap_has_space(size)) return nullptr;
    void* ptr = heap_ptr;
    heap_ptr += size;
    return ptr;
}

/// Allocate a new alloc_header.
__attribute__((malloc)) alloc_header* allocate_header() {
    alloc_header* header;

    /// Return the next free header if there is one.
    if (free_headers) {
        header = free_headers;
        free_headers = header->next;
        return header;
    }

    /// Otherwise, allocate a new one.
    header = reinterpret_cast<alloc_header*>(heap_alloc(sizeof(alloc_header)));

    /// Make sure to zero out the previous and next pointers.
    if (header) {
        header->next = nullptr;
        header->prev = nullptr;
    }

    /// Return the new header.
    return header;
}

/// Free an alloc_header.
void free_header(alloc_header* header) {
    header->next = free_headers;
    free_headers = header;
}

/// Find a block in the free list.
alloc_header* find_free_block(size_t size) {
    auto block = free_list;
    while (block) {
        if (block->size >= size) return block;
        block = block->next;
    }
    return nullptr;
}

/// Find a block in the allocated list.
alloc_header* find_alloc_block(void* ptr) {
    auto block = alloc_list;
    while (block) {
        if (block->ptr == ptr) return block;
        block = block->next;
    }
    return nullptr;
}

/// Align a number to the maximum possible alignment.
constexpr size_t align_to_max_align_t(size_t n) {
    constexpr size_t max_align = alignof(max_align_t);
    n = n ? (n + max_align - 1) & ~(max_align - 1) : max_align;
    return n;
}

/// Realloc implementation.
template <bool copy_data>
__attribute__((alloc_size(2))) void* realloc_impl(void* ptr, size_t size) {
    if (!ptr) { return malloc(size); }

    /// `realloc` with a size of 0 is implementation-defined. Just do nothing.
    if (!size) { return ptr; }

    /// Round up to the maximum alignment.
    size = align_to_max_align_t(size);

    /// If the pointer was most recently allocated, we can just
    /// extend or truncate the block.
    /// FIXME: I think this check is not correct.
/*    if (alloc_list && alloc_list->ptr == ptr) {
        /// Extend the block if possible and requested.
        if (size > alloc_list->size && heap_ptr - alloc_list->size + size <= heap_base + sizeof(heap_base)) {
            heap_ptr += size - alloc_list->size;
            alloc_list->size = size;
            return ptr;
        }

        /// Truncate the block if requested.
        if (size < alloc_list->size) {
            heap_ptr -= alloc_list->size - size;
            alloc_list->size = size;
            return ptr;
        }

        /// If the size is the same, do nothing.
        return ptr;
    }*/

    /// Find the block in the allocated list.
    auto block = find_alloc_block(ptr);
    __libc_assert(block, "realloc(): invalid pointer\n");

    /// If we're supposed to truncate the block, do nothing.
    if (size <= block->size) { return ptr; }

    /// Otherwise, allocate a new block.
    auto new_ptr = malloc(size);
    if (!new_ptr) { return nullptr; }

    /// Copy the data from the old block to the new block.
    if constexpr (copy_data) {
        memcpy(new_ptr, ptr, std::min(size, block->size));
    }

    /// Free the old block.
    free(ptr);

    /// Done!
    return new_ptr;
}

} // namespace

/// ===========================================================================
///  C Interface
/// ===========================================================================
__BEGIN_DECLS__
bool __stdio_destructed;

void __libc_init_malloc() {
    __errno = 0;
    __stdio_destructed = true;

    /// Initialise the heap.
    heap_ptr = heap_base;
    alloc_list = nullptr;
    free_list = nullptr;
    free_headers = nullptr;
}

void __libc_fini_malloc() {
    /// Currently, this does nothing, but one could, for instance, check
    /// for memory leaks here.
}

int* __errno_location(void) {
    return &__errno;
}

/// ===========================================================================
///  Assert and Exit
/// ===========================================================================
/// Report a failed assertion and abort.
__attribute__((__noreturn__)) void __assert_abort(
    const char *expr,
    const char *file,
    unsigned line,
    const char *func
) {
    if (!__stdio_destructed) {
        //fprintf(stderr, "%s: in function %s:%u: Assertion failed: %s\n", file, func, line, expr);
        fputs(file, stderr);
        fputs(_Y ": " _RR "assertion failed" _N _Y ": " _R, stderr);
        fputs(func, stderr);
        fputs(_Y "\n    in function " _G, stderr);
        fputs(expr, stderr);
        fputs(_N "\n", stderr);
        fflush(stderr);
    } else {
        write(2, "Assertion failed: ", 18);
        __write(expr);
        write(2, "\n", 1);
    }
    abort();
}

/// Report a failed assertion, print a message, and abort.
/// Report a failed assertion with a message.
__attribute__((__noreturn__)) void __assert_abort_msg(
    const char *expr,
    const char *msg,
    const char *file,
    unsigned int line,
    const char *func
) {
    if (!__stdio_destructed) {
        //fprintf(stderr, "%s: in function %s:%u: Assertion failed: %s: %s\n", file, func, line, expr, msg);
        fputs(file, stderr);
        fputs(_Y ": " _RR "assertion failed" _N _Y ": " _R, stderr);
        fputs(expr, stderr);
        fputs(_Y "\n    in function " _G, stderr);
        fputs(func, stderr);
        fputs(_Y "\n    message: " _B , stderr);
        fputs(msg, stderr);
        fputs(_N "\n", stderr);
        fflush(stderr);
    } else {
        write(2, "Assertion failed: ", 18);
        __write(expr);
        write(2, "\n", 1);
        write(2, "Message: ", 9);
        __write(msg);
        write(2, "\n", 1);
    }
    abort();
}

__attribute__((noreturn)) void __libc_exit(int);

__attribute__((noreturn)) void __libc_quick_exit(int);

__attribute__((noreturn)) void abort() { _Exit(-1); }

__attribute__((__noreturn__)) void exit(int status) {
    __libc_exit(status);
}

__attribute__((__noreturn__)) void quick_exit(int status) {
    __libc_quick_exit(status);
}

__attribute__((__noreturn__)) void _Exit(int status) {
    syscall(SYS_exit, status);
    for (;;) asm volatile ("");
}

int atexit(void function()) {
    __cxa_atexit([](void* _arg){
        if (__in_quick_exit) return;
        auto arg = reinterpret_cast<void(*)()>(_arg);
        arg();
    }, reinterpret_cast<void*>(function), nullptr);
    return 0;
}

int at_quick_exit(void function()) {
    __cxa_atexit(reinterpret_cast<__dso_cb>(function), nullptr, nullptr);
    return 0;
}

/// ===========================================================================
///  Malloc Interface.
/// ===========================================================================
__attribute__((malloc, alloc_size(1))) void* malloc(size_t bytes) {
    /// Round up to the maximum alignment.
    bytes = align_to_max_align_t(bytes);

    /// If we have a free block, use it.
    ///
    /// TODO: Use a better algorithm. This is certainly not the fastest way
    ///       of implementing a free list, though funnily enough it is optimised
    ///       for repeatedly freeing and allocating blocks of the same size.
    auto free_ptr = find_free_block(bytes);
    if (free_ptr) {
        /// Remove the block from the free list.
        if (free_ptr->prev) { free_ptr->prev->next = free_ptr->next; }
        if (free_ptr->next) { free_ptr->next->prev = free_ptr->prev; }
        if (free_list == free_ptr) { free_list = free_ptr->next; }

        /// Add the block to the allocated list.
        free_ptr->next = alloc_list;
        if (alloc_list) { alloc_list->prev = free_ptr; }
        alloc_list = free_ptr;

        return free_ptr->ptr;
    }

    /// We need to allocate a new block. Make sure we have enough space in the
    /// heap for both the block and the memory we need to allocate.
    static constexpr size_t block_sz = align_to_max_align_t(sizeof(alloc_header));
    if (heap_ptr + bytes + block_sz > heap_base + sizeof(heap_base)) {
        __errno = ENOMEM;
        return nullptr;
    }

    /// Allocate memory for the block.
    auto block = allocate_header();
    if (!block) { return nullptr; }

    /// Allocate the memory.
    auto ptr = heap_alloc(bytes);
    if (!ptr) {
        free_header(block);
        return nullptr;
    }

    /// Add the block to the allocated list.
    block->ptr = static_cast<char*>(ptr);
    block->size = bytes;
    block->next = alloc_list;
    if (alloc_list) { alloc_list->prev = block; }
    alloc_list = block;

    /// Return the pointer to the allocated memory.
    return ptr;
}
__attribute__((malloc, alloc_size(1, 2))) void* calloc(size_t n, size_t size) {
    auto ptr = malloc(n * size);
    if (ptr) { memset(ptr, 0, n * size); }
    return ptr;
}

__attribute__((alloc_size(2))) void* __mextend(void* ptr, size_t size) {
    return realloc_impl<false>(ptr, size);
}

__attribute__((alloc_size(2))) void* realloc(void* ptr, size_t size) {
    return realloc_impl<true>(ptr, size);
}

void free(void* ptr) {
    /// If the pointer is null, do nothing.
    if (!ptr) { return; }

    /// Find the block in the allocated list.
    auto block = find_alloc_block(ptr);

    /// If it's the most recently allocated block, just decrement the heap pointer.
    /// FIXME: I think this check is not correct.
/*    if (block == alloc_list) {
        heap_ptr -= block->size;
        alloc_list = block->next;
        if (alloc_list) { alloc_list->prev = nullptr; }
        free_header(block);
        return;
    }*/

    /// Make sure we have a valid pointer.
    if (!block) {
        auto head = free_list;
        while (head) {
            if (head->ptr == ptr) { __libc_assert(false, "free(): double free"); }
            head = head->next;
        }
        __libc_assert(false, "free(): invalid pointer");
    }

    /// Otherwise, remove the block from the allocated list.
    if (block->prev) { block->prev->next = block->next; }
    if (block->next) { block->next->prev = block->prev; }
    if (alloc_list == block) { alloc_list = block->next; }

    /// And add it to the free list.
    block->next = free_list;
    if (free_list) { free_list->prev = block; }
    free_list = block;
}

__END_DECLS__

[[nodiscard]] void* operator new(size_t size) { return malloc(size); }
[[nodiscard]] void* operator new[](size_t size) { return malloc(size); }
void operator delete(void* ptr) noexcept { free(ptr); }
void operator delete[](void* ptr) noexcept { free(ptr); }
void operator delete(void* ptr, size_t) noexcept { free(ptr); }
void operator delete[](void* ptr, size_t) noexcept { free(ptr); }
