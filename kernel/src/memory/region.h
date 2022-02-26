#ifndef LENSOR_OS_MEMORY_REGION_H
#define LENSOR_OS_MEMORY_REGION_H

#include "../integers.h"

namespace Memory {
    class Region {
    public:
        Region(void* baseAddress, u64 length)
            : BaseAddress(baseAddress), Length(length) {}

        Region(u64 baseAddress, u64 length)
            : BaseAddress((void*)baseAddress), Length(length) {}

        void* begin() { return BaseAddress; }
        void* end() { return (void*)((u64)BaseAddress + Length); }
        // NOTE: Length returns amount of pages in region, not bytes!
        u64 length() { return Length; }

        // Move this region to a specified address (rebase).
        void move_region(void* address) { BaseAddress = address; }
        // Grow the current region by a given amount of pages.
        void grow_region(u64 amount) { Length += amount; }

    private:
        // The byte address of the beginning of the memory region.
        void* BaseAddress { nullptr };
        // The length of the contiguous memory region starting at the base address, in pages.
        u64 Length;
    };
}

#endif
