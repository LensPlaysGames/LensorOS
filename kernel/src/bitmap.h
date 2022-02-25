#ifndef LENSOR_OS_BITMAP_H
#define LENSOR_OS_BITMAP_H

#include "integers.h"

class Bitmap {
public:
    Bitmap() {}

    Bitmap(u64 size, u8* bufferAddress)
        : Size(size), Buffer(bufferAddress) {}

    void init(u64 size, u8* bufferAddress) {
        Size = size;
        Buffer = bufferAddress;
    }

    u64 length() { return Size; }
    void* base() { return (void*)Buffer; };

    bool get(u64 index);
    bool set(u64 index, bool value);

    bool operator [] (u64 index);

private:
    /* Number of bits within the bitmap. */
    u64 Size;
    /* Buffer to store bitmap within. */
    u8* Buffer; 
};

#endif
