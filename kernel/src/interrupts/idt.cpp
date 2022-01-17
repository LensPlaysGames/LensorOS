#include "idt.h"

void IDTDescEntry::SetOffset(uint64_t offset) {
	offset0 = (uint16_t)(offset & 0x000000000000ffff);
	offset1 = (uint16_t)((offset & 0x00000000ffff0000) >> 16);
	offset2 = (uint16_t)((offset & 0xffffffff00000000) >> 32);
}
uint64_t GetOffset() {
	uint64_t offset = 0;
	offset |= (uint64_t)offset;
	offset |= (uint64_t)offset << 16;
	offset |= (uint64_t)offset << 32;
	return offset;
}
