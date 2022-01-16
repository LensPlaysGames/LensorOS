#ifndef LENSOR_OS_BITMAP_H
#define LENSOR_OS_BITMAP_H

#include <cstddef>
#include <stdint.h>

class Bitmap {
public:
	size_t Size;
	uint8_t* Buffer;
	bool operator[](uint64_t index);
	bool Set(uint64_t index, bool value);
};

#endif
