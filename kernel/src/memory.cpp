#include "memory.h"

uint64_t GetMemorySize(EFI_MEMORY_DESCRIPTOR* map, uint64_t mapEntries, uint64_t mapDescSize) {
	static uint64_t MemorySizeInBytes = 0;
	if (MemorySizeInBytes > 0) { return MemorySizeInBytes; }

	for (int i = 0; i < mapEntries; i++) {
		// Get descriptor for each map entry.
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)map + (i * mapDescSize));
		// Add memory size from descriptor to total memory size.
		// 4096 = page size in bytes
		MemorySizeInBytes += desc->numPages * 4096;
	}

	return MemorySizeInBytes;
}

void memset(void* start, uint8_t value, uint64_t numBytes) {
	for (uint64_t i = 0; i < numBytes; i++) {
		*(uint8_t*)((uint64_t)start + i) = value;
	}
}
