#ifndef LENSOR_OS_BITMAP_H
#define LENSOR_OS_BITMAP_H

#include "integers.h"

class Bitmap {
public:
    u64 Size;
    u8* Buffer;
    bool Get(u64 index);
    bool Set(u64 index, bool value);
    bool operator[](u64 index);
};

#endif
