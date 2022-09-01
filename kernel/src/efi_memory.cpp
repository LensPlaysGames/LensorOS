/* Copyright 2022, Contributors To LensorOS.
All rights reserved.

This file is part of LensorOS.

LensorOS is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

LensorOS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with LensorOS. If not, see <https://www.gnu.org/licenses */
#include <efi_memory.h>

#include <cstr.h>
#include <memory.h>
#include <memory/common.h>
#include <uart.h>

namespace Memory {
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
        "EfiPalCode",
    };

    void print_efi_memory_map(EFI_MEMORY_DESCRIPTOR* map, u64 mapSize, u64 mapDescSize) {
        u64 mapEntries = mapSize / mapDescSize;
        for (u64 i = 0; i < mapEntries; ++i) {
            EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)map + (i * mapDescSize));
            UART::out("\033[36m[MEMORY REGION]: ");
            if (desc->type < 14)
                UART::out(EFI_MEMORY_TYPE_STRINGS[desc->type]);
            else UART::out("\033[0mINVALID TYPE\033[36m");
            UART::out("\r\n  Physical Address: 0x");
            UART::out(to_hexstring<void*>(desc->physicalAddress));
            UART::out("\r\n  Size: ");
            u64 sizeKiB = desc->numPages * PAGE_SIZE / 1024;
            UART::out(to_string(sizeKiB / 1024));
            UART::out("MiB (");
            UART::out(to_string(sizeKiB));
            UART::out("KiB)\033[0m\r\n");
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
            UART::out("\033[36m[MEMORY REGION]: ");
            UART::out(EFI_MEMORY_TYPE_STRINGS[i]);
            UART::out("\r\n  Total Size: ");
            u64 sizeKiB = typePageSums[i] * PAGE_SIZE / 1024;
            UART::out(to_string(sizeKiB / 1024));
            UART::out("MiB (");
            UART::out(to_string(sizeKiB));
            UART::out("KiB)\033[0m\r\n");
        }
    }
}
