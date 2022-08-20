#include "stddef.h"
#include "stdlib.h"
#include "sys/syscalls.h"

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
