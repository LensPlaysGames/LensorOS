#include "basic_renderer.h"
#include "cstr.h"
#include "efi_memory.h"
#include "memory.h"
#include "bitmap.h"
#include "paging/paging.h"
#include "paging/page_table_manager.h"
#include "gdt.h"
#include "interrupts/idt.h"
#include "interrupts/interrupts.h"
#include "io.h"

struct BootInfo {
	Framebuffer* framebuffer;
	PSF1_FONT* font;
	EFI_MEMORY_DESCRIPTOR* map;
	uint64_t mapSize;
	uint64_t mapDescSize;
};

struct KernelInfo {
	PageTableManager* PTM;
};

extern uint64_t _KernelStart;
extern uint64_t _KernelEnd;

KernelInfo InitializeKernel(BootInfo* info);
