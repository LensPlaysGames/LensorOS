/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#ifndef LENSOR_OS_MEMORY_REGION_H
#define LENSOR_OS_MEMORY_REGION_H

#include <integers.h>

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
