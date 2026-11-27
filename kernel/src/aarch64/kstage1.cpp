#include <aarch64/device_tree.h>
#include <aarch64/uart.h>
#include <kstage1.h>
#include <memory/api.h>
#include <memory/common.h>
#include <memory/heap.h>
#include <memory/physical_memory_manager.h>
#include <panic.h>
#include <uart.h>

void kstage1(uintptr_t base, BootInfo* bInfo) {
    UART::Base = 0xc81004c0;
    UART::initialize();
    UART::out_raw('^');

    asm volatile(
        "1:\n\t"
        "wfe\n\t"
        "b 1b\n\t");

    KERNEL_PHYSICAL = (uintptr_t)base;

    // Setup physical memory allocator from EFI memory map.
    Memory::init_physical(
        bInfo->map,
        bInfo->mapSize,
        bInfo->mapDescSize);

    // using prekernel map, try to get a basic message out the UART
    fdt_header& fdt = *(fdt_header*)bInfo->devices;
    parse_device_tree(&fdt);

    hang();

    Memory::startup_handle = Memory::active_memory_space();

    // Setup dynamic memory allocation (`new`, `delete`).
    init_heap();

    // Informational/debug memory printout
    Memory::print_physmem();
    heap_print_debug_summed();

    dump_device_tree(&fdt);
}
