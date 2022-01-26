#include "idt.h"

void IDTDescEntry::SetOffset(u64 offset) {
	offset0 = (u16)(offset & 0x000000000000ffff);
	offset1 = (u16)((offset & 0x00000000ffff0000) >> 16);
	offset2 = (u32)((offset & 0xffffffff00000000) >> 32);
}

u64 IDTDescEntry::GetOffset() {
	u64 offset = 0;
	offset |= (u64)offset0;
	offset |= ((u64)offset1) << 16;
	offset |= ((u64)offset2) << 32;
	return offset;
}
