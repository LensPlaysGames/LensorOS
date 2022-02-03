#ifndef LENSOR_OS_PAGE_FRAME_ALLOCATOR
#define LENSOR_OS_PAGE_FRAME_ALLOCATOR

#include <cstddef>
#include "../integers.h"
#include "../efi_memory.h"
#include "../bitmap.h"
#include "../memory.h"
#include "../cstr.h"

class PageFrameAllocator {
public:
    PageFrameAllocator() {}
    
    void read_efi_memory_map(EFI_MEMORY_DESCRIPTOR* map, u64 mapSize, u64 mapDescSize);
    Bitmap PageBitmap;
    void free_page(void* addr);
    void lock_page(void* addr);
    void free_pages(void* addr, u64 numPages);
    void lock_pages(void* addr, u64 numPages);
    u64 get_free_ram();
    u64 get_used_ram();
    u64 get_reserved_ram();
    void* request_page();
    void* request_pages(u64 numPages);
private:
    void initialize_bitmap(u64 bitmapSize, void* bufAddress);
    void unreserve_page(void* addr);    
    void reserve_page(void* addr);
    void unreserve_pages(void* addr, u64 numPages);
    void reserve_pages(void* addr, u64 numPages);
};

extern PageFrameAllocator gAlloc;

#endif
