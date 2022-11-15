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
#include <memory/common.h>

namespace Memory {
    struct Region {
        void* vaddr = 0;
        void* paddr = 0;
        usz length  = 0;
        usz pages   = 0;
        u64 flags   = 0;

        Region(void* vaddress, void* paddress, usz bytes, u64 flag) {
            vaddr  = vaddress;
            paddr  = paddress;
            length = bytes;
            pages  = 1;
            pages += bytes / PAGE_SIZE;
            flags = flag;
        }
    };
}

#endif
