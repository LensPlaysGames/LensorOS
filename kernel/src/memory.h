#ifndef LENSOR_OS_MEMORY_H
#define LENSOR_OS_MEMORY_H

#include "efi_memory.h"

u64 get_memory_size(EFI_MEMORY_DESCRIPTOR* map, u64 mapEntries, u64 mapDescSize);

// Take in an address, `start`, and write `value` to the given number of bytes, `numBytes`.
void memset(void* start, u8 value, u64 numBytes);

void memcpy(void* start, void* dest, u64 numBytes);

#endif
