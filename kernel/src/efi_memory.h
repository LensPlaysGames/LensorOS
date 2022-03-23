#ifndef LENSOR_OS_EFI_MEMORY_OS
#define LENSOR_OS_EFI_MEMORY_OS

#include "integers.h"

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
