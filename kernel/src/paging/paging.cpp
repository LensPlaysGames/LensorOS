#include "paging.h"

void PageDirEntry::set_flag(PT_Flag flag, bool enabled) {
	uint64_t bitSelector = (uint64_t)1 << flag;
	Value &= ~bitSelector;
	if (enabled) {
		Value |= bitSelector;
	}
}

bool PageDirEntry::get_flag(PT_Flag flag) {
	uint64_t bitSelector = (uint64_t)1 << flag;
	return Value & bitSelector;
}

uint64_t PageDirEntry::get_address() {
	return (Value & 0x000ffffffffff000) >> 12;
}

void PageDirEntry::set_address(uint64_t addr) {
	addr &=  0x000000ffffffffff;
	Value &= 0xfff0000000000fff;
	Value |= (addr << 12);
}
