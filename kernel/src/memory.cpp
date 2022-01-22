#include "memory.h"

uint64_t get_memory_size(EFI_MEMORY_DESCRIPTOR* map, uint64_t mapEntries, uint64_t mapDescSize) {
	static uint64_t s_memory_size_in_bytes = 0;
	if (s_memory_size_in_bytes > 0) { return s_memory_size_in_bytes; }

	for (int i = 0; i < mapEntries; ++i) {
		// Get descriptor for each map entry.
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((uint64_t)map + (i * mapDescSize));
		// Add memory size from descriptor to total memory size.
		// 4096 = page size in bytes
	    s_memory_size_in_bytes += desc->numPages * 4096;
	}

	return s_memory_size_in_bytes;
}

void memset(void* start, uint8_t value, uint64_t numBytes) {
	for (uint64_t i = 0; i < numBytes; ++i) {
		*(uint8_t*)((uint64_t)start + i) = value;
	}
}

void memcpy(void* src, void* dest, uint64_t numBytes) {
	if (numBytes <= 256) {
		// Copy 8 byte segments instead of one byte at a time.
		uint64_t numQWords = numBytes / 8;
		uint8_t leftover = numBytes % 8;
		uint64_t i = 0;
		for (; i < numQWords; i += 8) {
			*(uint64_t*)((uint64_t)dest + i) = *(uint64_t*)((uint64_t)src + i);
		}
		// Finish up leftover bits.
		for (; i < numBytes; ++i) {
			*(uint8_t*)((uint64_t)dest + i) = *(uint8_t*)((uint64_t)src + i);
		}
	} else if (numBytes <= 16384){
		uint64_t numSWords = numBytes / 32;
		uint8_t leftover = numBytes % 32;
		uint64_t i = 0;
		for (; i < numSWords; i += 32) {
			*(uint256_t*)((uint64_t)dest + i) = *(uint256_t*)((uint64_t)src + i);
		}
		// Finish up leftover bits.
		for (; i < numBytes; ++i) {
			*(uint8_t*)((uint64_t)dest + i) = *(uint8_t*)((uint64_t)src + i);
		}
	} else {
		uint64_t numKWords = numBytes / 128;
		uint8_t leftoverBits = numBytes % 128;
		uint8_t leftoverQWords = leftoverBits / 8;
		leftoverBits = leftoverBits % 8;
		uint64_t i = 0;
		// Copy bulk of data using 1024 bit-wide chunks.
		for (; i < numKWords; i += 128) {
			*(uint1024_t*)((uint64_t)dest + i) = *(uint1024_t*)((uint64_t)src + i);
		}
		// Finish up leftover quad words.
		for (; i < numKWords + leftoverQWords; i += 8) {
			*(uint64_t*)((uint64_t)dest + i) = *(uint64_t*)((uint64_t)src + i);
		}
		// Finish up leftover bytes.
		for (; i < numBytes; ++i) {
			*(uint8_t*)((uint64_t)dest + i) = *(uint8_t*)((uint64_t)src + i);
		}
	}
}
