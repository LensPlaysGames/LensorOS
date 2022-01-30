#include "memory.h"
#include "large_integers.h"

u64 get_memory_size(EFI_MEMORY_DESCRIPTOR* map, u64 mapEntries, u64 mapDescSize) {
	static u64 s_memory_size_in_bytes = 0;
	if (s_memory_size_in_bytes > 0) { return s_memory_size_in_bytes; }

	for (u64 i = 0; i < mapEntries; ++i) {
		// Get descriptor for each map entry.
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)map + (i * mapDescSize));
		// Add memory size from descriptor to total memory size.
		// 4096 = page size in bytes
	    s_memory_size_in_bytes += desc->numPages * 4096;
	}

	return s_memory_size_in_bytes;
}

void memset(void* start, u8 value, u64 numBytes) {
	if (numBytes >= 256) {
		u64 qWordValue = 0;
		qWordValue |= (u64)value << 0;
		qWordValue |= (u64)value << 8;
		qWordValue |= (u64)value << 16;
		qWordValue |= (u64)value << 24;
		qWordValue |= (u64)value << 32;
		qWordValue |= (u64)value << 40;
		qWordValue |= (u64)value << 48;
		qWordValue |= (u64)value << 56;
		u64 i = 0;
		for (; i <= numBytes - 8; i += 8) {
			*(u64*)((u64)start + i) = qWordValue;
		}
		// Finish up leftover bits.
		for (; i < numBytes; ++i) {
			*(u8*)((u64)start + i) = value;
		}
	}
	for (u64 i = 0; i < numBytes; ++i) {
		*(u8*)((u64)start + i) = value;
	}
}

void memcpy(void* src, void* dest, u64 numBytes) {
	s64 i = 0;
	for (; i <= (s64)numBytes - 2048; i += 2048) {
		*(u16384*)((u64)dest + i) = *(u16384*)((u64)src + i);
	}
	for (; i <= (s64)numBytes - 128; i += 128) {
		*(u1024*)((u64)dest + i) = *(u1024*)((u64)src + i);
	}
	for (; i <= (s64)numBytes - 32; i += 32) {
		*(u256*)((u64)dest + i) = *(u256*)((u64)src + i);
	}
	for (; i <= (s64)numBytes - 8; i += 8) {
		*(u64*)((u64)dest + i) = *(u64*)((u64)src + i);
	}
	for (; i < (s64)numBytes; ++i) {
		*(u8*)((u64)dest + i) = *(u8*)((u64)src + i);
	}
}
