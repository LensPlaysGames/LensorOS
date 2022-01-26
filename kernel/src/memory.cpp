#include "memory.h"

u64 get_memory_size(EFI_MEMORY_DESCRIPTOR* map, u64 mapEntries, u64 mapDescSize) {
	static u64 s_memory_size_in_bytes = 0;
	if (s_memory_size_in_bytes > 0) { return s_memory_size_in_bytes; }

	for (int i = 0; i < mapEntries; ++i) {
		// Get descriptor for each map entry.
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)map + (i * mapDescSize));
		// Add memory size from descriptor to total memory size.
		// 4096 = page size in bytes
	    s_memory_size_in_bytes += desc->numPages * 4096;
	}

	return s_memory_size_in_bytes;
}

void memset(void* start, u8 value, u64 numBytes) {
	if (numBytes <= 256) {
		for (u64 i = 0; i < numBytes; ++i) {
			*(u8*)((u64)start + i) = value;
		}
	}
	else {
		u64 qWordValue = 0;
		qWordValue |= (u64)value << 0;
		qWordValue |= (u64)value << 8;
		qWordValue |= (u64)value << 16;
		qWordValue |= (u64)value << 24;
		qWordValue |= (u64)value << 32;
		qWordValue |= (u64)value << 40;
		qWordValue |= (u64)value << 48;
		qWordValue |= (u64)value << 56;
		u64 numQWords = numBytes / 8;
		u8 leftover = numBytes % 8;
		u64 i = 0;
		for (; i < numQWords; i += 8) {
			*(u64*)((u64)start + i) = qWordValue;
		}
		// Finish up leftover bits.
		for (; i < numBytes; ++i) {
			*(u8*)((u64)start + i) = value;
		}
	}
}

void memcpy(void* src, void* dest, u64 numBytes) {
	if (numBytes <= 256) {
		// Copy 8 byte segments instead of one byte at a time.
		u64 numQWords = numBytes / 8;
		u8 leftover = numBytes % 8;
		u64 i = 0;
		for (; i < numQWords; i += 8) {
			*(u64*)((u64)dest + i) = *(u64*)((u64)src + i);
		}
		// Finish up leftover bits.
		for (; i < numBytes; ++i) {
			*(u8*)((u64)dest + i) = *(u8*)((u64)src + i);
		}
	}
	else if (numBytes <= 16384){
		u64 numSWords = numBytes / 32;
		u8 leftover = numBytes % 32;
		u64 i = 0;
		for (; i < numSWords; i += 32) {
			*(u256*)((u64)dest + i) = *(u256*)((u64)src + i);
		}
		// Finish up leftover bits.
		for (; i < numBytes; ++i) {
			*(u8*)((u64)dest + i) = *(u8*)((u64)src + i);
		}
	}
	else {
		u64 numKWords = numBytes / 128;
		u8 leftoverBits = numBytes % 128;
		u8 leftoverQWords = leftoverBits / 8;
		leftoverBits = leftoverBits % 8;
		u64 i = 0;
		// Copy bulk of data using 1024 bit-wide chunks.
		for (; i < numKWords; i += 128) {
			*(u1024*)((u64)dest + i) = *(u1024*)((u64)src + i);
		}
		// Finish up leftover quad words.
		for (; i < numKWords + leftoverQWords; i += 8) {
			*(u64*)((u64)dest + i) = *(u64*)((u64)src + i);
		}
		// Finish up leftover bytes.
		for (; i < numBytes; ++i) {
			*(u8*)((u64)dest + i) = *(u8*)((u64)src + i);
		}
	}
}
