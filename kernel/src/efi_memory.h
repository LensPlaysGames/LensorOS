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
}

#endif
