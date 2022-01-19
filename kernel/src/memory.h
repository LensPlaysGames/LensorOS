#ifndef LENSOR_OS_MEMORY_H
#define LENSOR_OS_MEMORY_H

#include <stdint.h>
#include "efi_memory.h"

uint64_t GetMemorySize(EFI_MEMORY_DESCRIPTOR* map, uint64_t mapEntries, uint64_t mapDescSize);

// Take in an address, `start`, and write `value` to the given number of bytes, `numBytes`.
void memset(void* start, uint8_t value, uint64_t numBytes);

void memcpy(void* start, void* dest, uint64_t numBytes);

#endif
