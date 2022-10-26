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

#ifndef LENSOR_OS_PAGING_H
#define LENSOR_OS_PAGING_H

#include <integers.h>

namespace Memory {
    enum class PageTableFlag : u64 {
        Present       = 1ull << 0,
        ReadWrite     = 1ull << 1,
        UserSuper     = 1ull << 2,
        WriteThrough  = 1ull << 3,
        CacheDisabled = 1ull << 4,
        Accessed      = 1ull << 5,
        Dirty         = 1ull << 6,
        LargerPages   = 1ull << 7,
        Global        = 1ull << 8,
        NX            = 1ull << 63,
    };

    class PageMapIndexer {
    public:
        explicit PageMapIndexer(u64 virtualAddress) {
            virtualAddress >>= 12;
            PageIndex =                 virtualAddress & 0x1ff;
            virtualAddress >>= 9;
            PageTableIndex =            virtualAddress & 0x1ff;
            virtualAddress >>= 9;
            PageDirectoryIndex =        virtualAddress & 0x1ff;
            virtualAddress >>= 9;
            PageDirectoryPointerIndex = virtualAddress & 0x1ff;
        }

        u64 page_directory_pointer() {
            return PageDirectoryPointerIndex;
        }

        u64 page_directory() {
            return PageDirectoryIndex;
        }

        u64 page_table() {
            return PageTableIndex;
        }

        u64 page() {
            return PageIndex;
        }

    private:
        u64 PageDirectoryPointerIndex;
        u64 PageDirectoryIndex;
        u64 PageTableIndex;
        u64 PageIndex;
    };

    class PageDirectoryEntry {
    public:
        u64 address()  {
            return (Value & 0x000ffffffffff000) >> 12;
        }

        void set_address(u64 addr)  {
            addr &=  0x000000ffffffffff;
            Value &= 0xfff0000000000fff;
            Value |= (addr << 12);
        }

        bool flag(PageTableFlag flag)  {
            return Value & (u64)flag;
        }

        void set_flag(PageTableFlag flag, bool enabled) {
            u64 bitSelector = (u64)flag;
            Value &= ~bitSelector;
            if (enabled) {
                Value |= bitSelector;
            }
        }

    private:
        u64 Value { 0 };
    } __attribute__((packed));

    struct PageTable {
        PageDirectoryEntry entries[512];
    } __attribute__((aligned(0x1000)));
}

#endif /* LENSOR_OS_PAGING_H */
