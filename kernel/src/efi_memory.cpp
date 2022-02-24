#include "efi_memory.h"

#include "cstr.h"
#include "memory.h"
#include "uart.h"

void print_efi_memory_map(EFI_MEMORY_DESCRIPTOR* map, u64 mapSize, u64 mapDescSize) {
    u64 mapEntries = mapSize / mapDescSize;
    for (u64 i = 0; i < mapEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)map + (i * mapDescSize));
        srl->writestr("\033[36m[MEMORY REGION]: ");
        if (desc->type < 14)
            srl->writestr(EFI_MEMORY_TYPE_STRINGS[desc->type]);
        else srl->writestr("\033[0mINVALID TYPE\033[36m");
        srl->writestr("\r\n  Physical Address: 0x");
        srl->writestr(to_hexstring((u64)desc->physicalAddress));
        srl->writestr("\r\n  Size: ");
        u64 sizeKiB = desc->numPages * 4096 / 1024;
        srl->writestr(to_string(sizeKiB / 1024));
        srl->writestr("MiB (");
        srl->writestr(to_string(sizeKiB));
        srl->writestr("KiB)\033[0m\r\n");
    }
}

void print_efi_memory_map_summed(EFI_MEMORY_DESCRIPTOR* map, u64 mapSize, u64 mapDescSize) {
    u64 mapEntries = mapSize / mapDescSize;
    u64 typePageSums[14];
    // Zero out sums to ensure a known starting point.
    memset(&typePageSums[0], 0, 14 * sizeof(u64));
    for (u64 i = 0; i < mapEntries; ++i) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)map + (i * mapDescSize));
        if (desc->type < 14)
            typePageSums[desc->type] += desc->numPages;
    }
    for (u8 i = 0; i < 14; ++i) {
        srl->writestr("\033[36m[MEMORY REGION]: ");
        srl->writestr(EFI_MEMORY_TYPE_STRINGS[i]);
        srl->writestr("\r\n  Total Size: ");
        u64 sizeKiB = typePageSums[i] * 4096 / 1024;
        srl->writestr(to_string(sizeKiB / 1024));
        srl->writestr("MiB (");
        srl->writestr(to_string(sizeKiB));
        srl->writestr("KiB)\033[0m\r\n");
    }
}

const char* EFI_MEMORY_TYPE_STRINGS[] {
    "EfiReservedMemoryType",
    "EfiLoaderCode",
    "EfiLoaderData",
    "EfiBootServicesCode",
    "EfiBootServicesData",
    "EfiRuntimeServicesCode",
    "EfiRuntimeServicesData",
    "EfiConventionalMemory",
    "EfiUnusableMemory",
    "EfiACPIReclaimMemory",
    "EfiACPIMemoryNVS",
    "EfiMemoryMappedIO",
    "EfiMemoryMappedIOPortSpace",
    "EfiPalCode"
};
