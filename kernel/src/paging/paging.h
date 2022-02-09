#ifndef LENSOR_OS_PAGING_H
#define LENSOR_OS_PAGING_H

#include "../integers.h"

/*
The idea is to create a tree of nodes that can be traversed to get a page of memory.
A 'page' is equal to 4096 bytes (4 kB) of memory.
A Modern OS gives each program virtual addresses that start at
  zero for ease of use, however the physical address of each page of
  memory can vary wildly.
The virtual address is an address that is used to lookup the physical
  address of a page within the page map level four table (see PML4 TABLE example below).

PML4 TABLE [512]
|\
| Page Directory Pointer Table [512]
| |\
| | Page Directory Table [512]
| | |\
| | | Page Table [512]
| | | |\
| | | | Page
| | | |\
| | | | Page
| | |  \
| | |   Page...
| | |\
| | | Page Table [512]
| | | |\
| | | | Page
| | | |\
| | | | Page
| | |  \
| | |   Page...
| |  \
| |   Page Table [512]
| |   |\
| |   | Page
| |   |\
| |   | Page
| |    \
| |     Page...
| \
|  Page Directory Table [512]...
|\
| Page Directory Pointer Table [512]
| |\
| | Page Directory Table [512]
| | |\
| | | Page Table [512]
| | | |\
| | | | Page
| | | |\
| | | | Page
| | |  \
| | |   Page...
| | |\
| | | Page Table [512]
| | | |\
| | | | Page
| | | |\
| | | | Page
| | |  \
| | |   Page...
| |  \
| |   Page Table [512]
| |   |\
| |   | Page
| |   |\
| |   | Page
| |    \
| |     Page...
| \
|  Page Directory Table [512]...
\
 Page Directory Pointer Table [512]...
*/
struct PageMapIndexer {
    u64 PageDirectoryPointerIndex;
    u64 PageDirectoryIndex;
    u64 PageTableIndex;
    u64 PageIndex;

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
};

enum PT_Flag {
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

struct PageDirEntry {
    u64 Value {0};
    
    void set_flag(PT_Flag flag, bool enabled);
    bool get_flag(PT_Flag flag);
    void set_address(u64 addr);
    u64 get_address();
} __attribute__((packed));

struct PageTable {
  PageDirEntry entries[512];
}__attribute__((aligned(0x1000)));

#endif
