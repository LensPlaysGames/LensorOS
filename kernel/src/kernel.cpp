// Custom includes
#include "BasicRenderer.h"
#include "cstr.h"
#include "efi_memory.h"
#include "memory.h"
#include "bitmap.h"
#include "paging/paging.h"
#include "paging/page_table_manager.h"

struct BootInfo {
  Framebuffer* framebuffer;
  PSF1_FONT* font;
  EFI_MEMORY_DESCRIPTOR* map;
  uint64_t mapSize;
  uint64_t mapDescSize;
};

extern uint64_t _KernelStart;
extern uint64_t _KernelEnd;

// TODO:
// - Cleanup kernel with kernelUtil (call it kUtility or something at least bareable).
//     [kernelUtil video resource](https://youtu.be/mh7pQ_E-OTw?t=762)
// - GDT & an assembler
// - Interrupts, fault handlers, etc
// - Panic Screen (blue screen)
// - IO bus
// - Keyboard
// - Mouse
// - Mouse Cursor


extern "C" void _start(BootInfo* info) {
	// Setup GOP renderer
	BasicRenderer rend(info->framebuffer, info->font);
	// Clear screen to background color (removes UEFI bootloader messages)
	rend.clear();

	rend.putstr("Setting up PageFrameAllocator from EFI memory map");
	rend.crlf();
	
	// Setup global page frame allocator
	gAlloc = PageFrameAllocator();
	// Setup memory map
	gAlloc.ReadEfiMemoryMap(info->map, info->mapSize, info->mapDescSize);
	
	rend.putstr("PageFrameAllocator setup successful");
	rend.crlf();

	rend.putstr("Allocating memory for kernel");
	rend.crlf();

	uint64_t kernelSize = (uint64_t)&_KernelEnd - (uint64_t)&_KernelStart;
	uint64_t kernelPagesNeeded = (uint64_t)kernelSize / 4096 + 1;
	gAlloc.LockPages(&_KernelStart, kernelPagesNeeded);

	rend.putstr("Memory for kernel allocated (");
	rend.putstr(to_string(kernelPagesNeeded * 4));
	rend.putstr(" kB)");
	rend.crlf();
	
	rend.crlf();
	gAlloc.PrintMemoryInfo(&rend);

	// PAGE MAP LEVEL FOUR (see paging.h).
    PageTable* PML4 = (PageTable*)gAlloc.RequestPage();
	memset(PML4, 0, 0x1000);
	gAlloc.LockPages(PML4, 1000);
	PageTableManager PTM(PML4);
	// Map all of the memory.
	for (uint64_t t = 0; t < GetMemorySize(info->map, info->mapSize / info->mapDescSize, info->mapDescSize); t+=0x1000) {
		PTM.MapMemory((void*)t, (void*)t);
	}
	// Map framebuffer memory.
	uint64_t fbBase = (uint64_t)info->framebuffer->BaseAddress;
	uint64_t fbSize = (uint64_t)info->framebuffer->BufferSize + 0x1000;
	gAlloc.LockPages((void*)fbBase, fbSize / 0x1000 + 1);
	for (uint64_t t = fbBase; t < fbBase + fbSize; t += 0x1000) {
		PTM.MapMemory((void*)t, (void*)t);
	}
	
	// Load PML4 into correct register.
	asm ("mov %0, %%cr3" : : "r" (PML4));

	rend.clear();

	rend.putstr("This is the test that I don't think I'll pass");
	rend.crlf();

	gAlloc.PrintMemoryInfo(&rend);
	
	// left eye
	rend.PixelPosition = {420, 420};
	rend.putrect({42, 42}, 0xff00ffff);
	// left pupil
	rend.PixelPosition = {440, 440};
	rend.putrect({20, 20}, 0xffff0000);
	// right eye
	rend.PixelPosition = {520, 420};
	rend.putrect({42, 42}, 0xff00ffff);
	// right pupil
	rend.PixelPosition = {540, 440};
	rend.putrect({20, 20}, 0xffff0000);
	// mouth
	rend.PixelPosition = {400, 520};
	rend.putrect({182, 20}, 0xff00ffff);
}
