#include "bitmap.h"

bool Bitmap::operator[](uint64_t index) {
	if (index >= Size * 8) { return false; }
	if ((Buffer[index / 8] & (0b10000000 >> (index % 8))) > 0) {
		return true;
	}
	return false;
}

bool Bitmap::Set(uint64_t index, bool value) {
	if (index >= Size * 8) { return false; }
	uint64_t byteIndex = index / 8;
	uint8_t bitIndexer = 0b10000000 >> (index % 8);
	Buffer[byteIndex] &= ~bitIndexer;
	if (value) {
		Buffer[byteIndex] |= bitIndexer;
	}
	return true;
}
