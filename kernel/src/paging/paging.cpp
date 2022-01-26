#include "paging.h"

void PageDirEntry::set_flag(PT_Flag flag, bool enabled) {
	u64 bitSelector = (u64)1 << flag;
	Value &= ~bitSelector;
	if (enabled) {
		Value |= bitSelector;
	}
}

bool PageDirEntry::get_flag(PT_Flag flag) {
	u64 bitSelector = (u64)1 << flag;
	return Value & bitSelector;
}

u64 PageDirEntry::get_address() {
	return (Value & 0x000ffffffffff000) >> 12;
}

void PageDirEntry::set_address(u64 addr) {
	addr &=  0x000000ffffffffff;
	Value &= 0xfff0000000000fff;
	Value |= (addr << 12);
}
