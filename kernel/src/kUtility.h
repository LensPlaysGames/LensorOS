// GRAPHICS
#include "basic_renderer.h"
// BITMAP ABSTRACTION (for usability, NOT performance)
#include "bitmap.h"
// to_string, to_hexstring
#include "cstr.h"
// MEMORY MANAGEMENT
#include "efi_memory.h"
#include "memory.h"
#include "paging/paging.h"
#include "paging/page_table_manager.h"
#include "heap.h"
// GLOBAL DESCRIPTOR TABLE
#include "gdt.h"
// INTERRUPTS
#include "interrupts/idt.h"
#include "interrupts/interrupts.h"
// SERIAL BUS COMMUNICATION
#include "io.h"
// TIMING
#include "timer.h"
#include "rtc.h"
// SYSTEM TABLES
#include "acpi.h"
#include "pci.h"

struct BootInfo {
	Framebuffer* framebuffer;
	PSF1_FONT* font;
	EFI_MEMORY_DESCRIPTOR* map;
	uint64_t mapSize;
	uint64_t mapDescSize;
	ACPI::RSDP2* rsdp;
};

struct KernelInfo {
	PageTableManager* PTM;
};

extern uint64_t _KernelStart;
extern uint64_t _KernelEnd;

KernelInfo kernel_init(BootInfo* info);
