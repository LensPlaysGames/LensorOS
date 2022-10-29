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


#include "stddef.h"
#include "stdlib.h"
#include "sys/syscalls.h"

/// This MUST be thread-local.
thread_local int __errno = 0;

extern "C" int* __errno_location(void) {
    return &__errno;
}

__attribute__((malloc, alloc_size(1))) void* malloc(size_t byteCount) {
    // TODO: Implement me!
    return nullptr;
}
__attribute__((malloc, alloc_size(1, 2))) void* calloc(size_t numberOfItems, size_t bytesPerItem) {
    // TODO: Implement me!
    return nullptr;
}

// NOTE: If byteCount is zero and ptr points to an already-allocated
// block, free the memory pointed to by ptr and return NULL.
__attribute__((alloc_size(2))) void* realloc(void* ptr, size_t byteCount) {
    // TODO: Implement me!
    if (ptr == NULL) {
        return malloc(byteCount);
    }
    return nullptr;
}

__attribute__((noreturn)) void abort() {
    syscall(SYS_exit, -1);
    while (1);
}
