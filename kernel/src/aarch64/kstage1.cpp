#include <aarch64/device_tree.h>
#include <kstage1.h>
#include <memory/api.h>
#include <memory/common.h>
#include <memory/heap.h>
#include <memory/physical_memory_manager.h>
#include <panic.h>
#include <uart.h>

void kstage1(uintptr_t base, BootInfo* bInfo) {
    KERNEL_PHYSICAL = (uintptr_t)base;

    // using prekernel map, try to get a basic message out the UART
    fdt_header& fdt = *(fdt_header*)bInfo->devices;
    parse_device_tree(&fdt);

    hang();

    // Setup physical memory allocator from EFI memory map.
    Memory::init_physical(
        bInfo->map,
        bInfo->mapSize,
        bInfo->mapDescSize);

    Memory::startup_handle = Memory::active_memory_space();

    // Informational/debug physical memory printout
    Memory::print_physmem();

    // Setup dynamic memory allocation (`new`, `delete`).
    init_heap();
    heap_print_debug_summed();

    dump_device_tree(&fdt);

    hang();
}
