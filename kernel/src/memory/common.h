#ifndef LENSOR_OS_MEMORY_COMMON_H
#define LENSOR_OS_MEMORY_COMMON_H

#include "../integers.h"

#define KB(x) ((u64)(x) << 10)
#define MB(x) ((u64)(x) << 20)
#define GB(x) ((u64)(x) << 30)

constexpr u64 PAGE_SIZE = 4096;

#endif /* LENSOR_OS_MEMORY_COMMON_H */
