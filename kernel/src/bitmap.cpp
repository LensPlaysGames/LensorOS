#include "bitmap.h"

#include "integers.h"
#include "memory.h"

Bitmap::Bitmap(u64 size, u8* bufferAddress)
    : Size(size), Buffer(bufferAddress)
{
    memset(Buffer, 0, Size);
}

void Bitmap::init(u64 size, u8* bufferAddress) {
    Size = size;
    Buffer = bufferAddress;
    // Initialize the buffer to all zeros (ensure known state).
    memset(Buffer, 0, Size);
}

bool Bitmap::get(u64 index) {
    u64 byteIndex = index / 8;
    if (byteIndex >= Size)
        return false;

    u8 bitIndexer = 0b10000000 >> (index % 8);
    return (Buffer[byteIndex] & bitIndexer) > 0;
}

bool Bitmap::set(u64 index, bool value) {
    u64 byteIndex = index / 8;
    if (byteIndex >= Size)
        return false;

    u8 bitIndexer = 0b10000000 >> (index % 8);
    Buffer[byteIndex] &= ~bitIndexer;
    if (value)
        Buffer[byteIndex] |= bitIndexer;

    return true;
}

bool Bitmap::operator[](u64 index) {
    return get(index);
}
