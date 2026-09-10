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

#include <print>

namespace Memory {
enum class PageTableFlag : u64 {
    Present = u64(1) << 0,
    ReadWrite = u64(1) << 1,
    UserSuper = u64(1) << 2,
    WriteThrough = u64(1) << 3,
    CacheDisabled = u64(1) << 4,
    Accessed = u64(1) << 5,
    Dirty = u64(1) << 6,
    LargerPages = u64(1) << 7,
    Global = u64(1) << 8,
    Lensor_CopyOnWrite = u64(1) << 9,
    NX = u64(1) << 63,
};

class PageMapIndexer {
   public:
    explicit PageMapIndexer(u64 virtualAddress) {
        virtualAddress >>= 12;
        PageIndex = virtualAddress & 0x1ff;
        virtualAddress >>= 9;
        PageTableIndex = virtualAddress & 0x1ff;
        virtualAddress >>= 9;
        PageDirectoryIndex = virtualAddress & 0x1ff;
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
    u64 address_huge() {
        return Value & 0x000fffffc0000000;
    }
    u64 address_large() {
        return Value & 0x000fffffffe00000;
    }
    u64 address() {
        return Value & 0x000ffffffffff000;
    }

    void set_address_huge(u64 addr) {
        addr &= 0x000fffffc0000000;
        Value &= 0xfff000003fffffff;
        Value |= addr;
    }

    void set_address_large(u64 addr) {
        addr &= 0x000fffffffe00000;
        Value &= 0xfff00000001fffff;
        Value |= addr;
    }

    void set_address(u64 addr) {
        if (flag(PageTableFlag::LargerPages)) {
            std::print("[PAGING]:ERROR: set_address() called on large page (use set_address_{{large,huge}}))\n");
        }
        addr &= 0x000ffffffffff000;
        Value &= 0xfff0000000000fff;
        Value |= addr;
    }

    bool flag(PageTableFlag flag) {
        return Value & (u64)flag;
    }

    u64 flags() {
        return Value & 0xfff0000000000fff;
    }

    void set_flag(PageTableFlag flag, bool enabled) {
        u64 bitSelector = (u64)flag;
        Value &= ~bitSelector;
        if (enabled) Value |= bitSelector;
    }

    // Or the given flag with the PDE value. This is used to set a
    // flag if it's unset, but do nothing if it is already set.
    void or_flag(PageTableFlag flag) {
        Value |= (u64)flag;
    }

    void or_flag_if(PageTableFlag flag, bool enabled) {
        if (enabled) Value |= (u64)flag;
    }

   private:
    u64 Value{0};
} __attribute__((packed));

struct PageTable {
    PageDirectoryEntry entries[512];
} __attribute__((aligned(0x1000)));
}  // namespace Memory

#endif /* LENSOR_OS_PAGING_H */
