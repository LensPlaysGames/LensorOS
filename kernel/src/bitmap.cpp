#include "bitmap.h"

bool Bitmap::Get(u64 index) {
	if (index >= Size * 8) { return false; }
	if ((Buffer[index / 8] & (0b10000000 >> (index % 8))) > 0) {
		return true;
	}
	return false;
}

bool Bitmap::Set(u64 index, bool value) {
	if (index >= Size * 8) { return false; }
	u64 byteIndex = index / 8;
	u8 bitIndexer = 0b10000000 >> (index % 8);
	Buffer[byteIndex] &= ~bitIndexer;
	if (value) {
		Buffer[byteIndex] |= bitIndexer;
	}
	return true;
}

bool Bitmap::operator[](u64 index) {
    return Get(index);
}
