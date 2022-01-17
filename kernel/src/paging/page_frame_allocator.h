#ifndef LENSOR_OS_PAGE_FRAME_ALLOCATOR
#define LENSOR_OS_PAGE_FRAME_ALLOCATOR

#include <stdint.h>
#include "../efi_memory.h"
#include "../bitmap.h"
#include "../memory.h"
#include "../basic_renderer.h"
#include "../cstr.h"

class PageFrameAllocator {
public:
	PageFrameAllocator() {}
	
	void ReadEfiMemoryMap(EFI_MEMORY_DESCRIPTOR* map, size_t mapSize, size_t mapDescSize);
	Bitmap PageBitmap;
	void FreePage(void* addr);
	void LockPage(void* addr);
	void FreePages(void* addr, uint64_t numPages);
	void LockPages(void* addr, uint64_t numPages);
	uint64_t GetFreeRAM();
	uint64_t GetUsedRAM();
	uint64_t GetReservedRAM();
    void* RequestPage();
	// FIXME: Early debug purposes only!
	void PrintMemoryInfo(BasicRenderer* rend);
private:
	void InitializeBitmap(size_t bitmapSize, void* buf_address);
	void UnreservePage(void* addr);	
	void ReservePage(void* addr);
	void UnreservePages(void* addr, uint64_t numPages);
	void ReservePages(void* addr, uint64_t numPages);
};

extern PageFrameAllocator gAlloc;

#endif
