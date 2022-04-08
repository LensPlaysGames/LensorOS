#ifndef LENSOR_OS_BOOT_H
#define LENSOR_OS_BOOT_H

#include <acpi.h>
#include <basic_renderer.h>
#include <efi_memory.h>
#include <integers.h>

struct BootInfo {
    Framebuffer* framebuffer;
    PSF1_FONT* font;
    EFI_MEMORY_DESCRIPTOR* map;
    u64 mapSize;
    u64 mapDescSize;
    ACPI::RSDP2* rsdp;
};

#endif /* LENSOR_OS_BOOT_H */
