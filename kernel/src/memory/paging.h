#ifndef LENSOR_OS_PAGING_H
#define LENSOR_OS_PAGING_H

#include <integers.h>

namespace Memory {
    enum PageTableFlag {
        Present = 0,
        ReadWrite = 1,
        UserSuper = 2,
        WriteThrough = 3,
        CacheDisabled = 4,
        Accessed = 5,
        Dirty = 6,
        LargerPages = 7,
        Global = 8,
        NX = 63,
    };

    class PageMapIndexer {
    public:
        PageMapIndexer(u64 virtualAddress) {
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
        u64 get_address()  {
            return (Value & 0x000ffffffffff000) >> 12;
        }
        
        void set_address(u64 addr)  {
            addr &=  0x000000ffffffffff;
            Value &= 0xfff0000000000fff;
            Value |= (addr << 12);
        }
        
        bool get_flag(PageTableFlag flag)  {
            return Value & (u64)1 << flag;
        }
        
        void set_flag(PageTableFlag flag, bool enabled) {
            u64 bitSelector = (u64)1 << flag;
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
