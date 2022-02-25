#ifndef LENSOR_OS_MEMORY_H
#define LENSOR_OS_MEMORY_H

#include "integers.h"

void memset(void* src, u8 value  , u64 numBytes);
void memcpy(void* src, void* dest, u64 numBytes);

void volatile_read(const volatile void* ptr, volatile void* out, u64 length);
void volatile_write(void* data, volatile void* ptr, u64 length);

#endif
