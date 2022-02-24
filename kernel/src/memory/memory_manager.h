#ifndef LENSOR_OS_MEMORY_MANAGER_H
#define LENSOR_OS_MEMORY_MANAGER_H

#include "../bitmap.h"
#include "../efi_memory.h"
#include "../linked_list.h"
#include "region.h"

namespace Memory {
    struct ManagedRegion {
        Region region;
        Bitmap bitmap;

        ManagedRegion(Region r) : region(r) {}
    };

    /* Returns the amount of free ram in bytes. */
    u64 get_free_ram();
    /* Returns the amount of used ram in bytes. */
    u64 get_used_ram();

    bool is_initialized();
    /* Read and parse an array of EFI_MEMORY_DESCRIPTORs into
     *   a singly linked list of free regions that may be used.
     */
    void init_efi(EFI_MEMORY_DESCRIPTOR* map, u64 size, u64 entrySize);

    void print_debug();
}

#endif /* LENSOR_OS_MEMORY_MANAGER_H */
