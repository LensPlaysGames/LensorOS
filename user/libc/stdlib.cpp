#include "stdlib.h"

__attribute__((malloc, alloc_size(1))) void* malloc(size_t) {
    // TODO: Implement me!
    return nullptr;
}
__attribute__((malloc, alloc_size(1, 2))) void* calloc(size_t nitems, size_t) {
    // TODO: Implement me!
    return nullptr;
}
__attribute__((alloc_size(2))) void* realloc(void* ptr, size_t) {
    // TODO: Implement me!
    return nullptr;
}

__attribute__((noreturn)) void exit(int status) {
    // TODO: Implement me!
    while (1);
}

__attribute__((noreturn)) void abort(void) {
    // TODO: Implement me!  
    while (1);
}
