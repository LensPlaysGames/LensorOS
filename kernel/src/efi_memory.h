/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#ifndef LENSOR_OS_EFI_MEMORY_OS
#define LENSOR_OS_EFI_MEMORY_OS

#include <integers.h>

namespace EFI {

enum struct MemoryDescriptorType {
    Reserved = 0,
    LoaderCode = 1,
    LoaderData = 2,
    BootServicesCode = 3,
    BootServicesData = 4,
    RuntimeServicesCode = 5,
    RuntimeServicesData = 6,
    ConventionalMemory = 7,
    UnusableMemory = 8,
    ACPIReclaimMemory = 9,
    ACPIMemoryNVS = 10,
    MemoryMappedIO = 11,
    MemoryMappedIOPortSpace = 12,
    PalCode = 13,
    Persistent = 14,

    /** UEFI mandates that types within the range 0x70000000 - 0x7fffffff
     ** are reserved for OEM (hardware manufacturer) use. Don't touch.
     **/
    RangeBeginOEM = 0x70000000,
    RangeEndOEM = 0x7fffffff,

    /** Technically, UEFI mandates that types above 0x80000000 are reserved
     ** for OS and vendor-specific use. Not often supported in hardware.
     **/
    RangeBeginOSVendor = 0x70000000,
    RangeEndOSVendor = 0x7fffffff,
};

struct MemoryDescriptor {
    MemoryDescriptorType type;
    u32 pad;
    void* physical_address;
    void* virtual_address;
    u64 page_count;
    u64 attributes;
};

}  // namespace EFI

// TODO: Move this struct into Memory namespace.
struct EFI_MEMORY_DESCRIPTOR {
    u32 type;
    u32 pad;
    void* physicalAddress;
    void* virtualAddress;
    u64 numPages;
    u64 attributes;
};

namespace Memory {
extern const char* EFI_MEMORY_TYPE_STRINGS[];

void print_efi_memory_map(EFI_MEMORY_DESCRIPTOR* map, u64 mapSize, u64 entrySize);
void print_efi_memory_map_summed(EFI_MEMORY_DESCRIPTOR* map, u64 mapSize, u64 entrySize);
}  // namespace Memory

#endif
