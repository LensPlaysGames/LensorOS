#ifndef LENSOR_OS_MEMORY_COMMON_H
#define LENSOR_OS_MEMORY_COMMON_H

#include <integers.h>

#define KiB(x) ((u64)(x) << 10)
#define MiB(x) ((u64)(x) << 20)
#define GiB(x) ((u64)(x) << 30)

#define TO_KiB(x) ((u64)(x) >> 10)
#define TO_MiB(x) ((u64)(x) >> 20)
#define TO_GiB(x) ((u64)(x) >> 30)

constexpr u64 PAGE_SIZE = 4096;

#endif /* LENSOR_OS_MEMORY_COMMON_H */
