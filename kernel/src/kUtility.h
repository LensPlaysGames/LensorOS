// GRAPHICS
#include "basic_renderer.h"
// BITMAP ABSTRACTION
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
#include "interrupts/syscalls.h"
// SERIAL BUS COMMUNICATION
#include "io.h"
// TIMING
#include "pit.h"
#include "rtc.h"
// SYSTEM TABLES
#include "acpi.h"
#include "pci.h"
// SERIAL COMMUNICATIONS
#include "uart.h"
// FILESYSTEM/DISK DRIVERS
#include "FATDriver.h"

struct BootInfo {
    Framebuffer* framebuffer;
    PSF1_FONT* font;
    EFI_MEMORY_DESCRIPTOR* map;
    u64 mapSize;
    u64 mapDescSize;
    ACPI::RSDP2* rsdp;
};

struct KernelInfo {
    PageTableManager* PTM;
};

extern u64 _KernelStart;
extern u64 _KernelEnd;

extern u64 _TextStart;
extern u64 _TextEnd;
extern u64 _DataStart;
extern u64 _DataEnd;
extern u64 _ReadOnlyDataStart;
extern u64 _ReadOnlyDataEnd;
extern u64 _BlockStartingSymbolsStart;
extern u64 _BlockStartingSymbolsEnd;

KernelInfo kernel_init(BootInfo* info);
