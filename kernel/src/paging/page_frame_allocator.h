#ifndef LENSOR_OS_PAGE_FRAME_ALLOCATOR
#define LENSOR_OS_PAGE_FRAME_ALLOCATOR

#include <stdint.h>
#include "../efi_memory.h"
#include "../bitmap.h"
#include "../memory.h"
#include "../cstr.h"

class PageFrameAllocator {
public:
	PageFrameAllocator() {}
	
	void read_efi_memory_map(EFI_MEMORY_DESCRIPTOR* map, size_t mapSize, size_t mapDescSize);
	Bitmap PageBitmap;
	void free_page(void* addr);
	void lock_page(void* addr);
	void free_pages(void* addr, uint64_t numPages);
	void lock_pages(void* addr, uint64_t numPages);
	uint64_t get_free_ram();
	uint64_t get_used_ram();
	uint64_t get_reserved_ram();
    void* request_page();
    void* request_pages(uint64_t numPages);
private:
	void initialize_bitmap(size_t bitmapSize, void* bufAddress);
	void unreserve_page(void* addr);	
	void reserve_page(void* addr);
	void unreserve_pages(void* addr, uint64_t numPages);
	void reserve_pages(void* addr, uint64_t numPages);
};

extern PageFrameAllocator gAlloc;

#endif
