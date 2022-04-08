#ifndef LENSOR_OS_PHYSICAL_MEMORY_MANAGER_H
#define LENSOR_OS_PHYSICAL_MEMORY_MANAGER_H

#include <bitmap.h>
#include <efi_memory.h>
#include <linked_list.h>

namespace Memory {
    void init_physical(EFI_MEMORY_DESCRIPTOR* map, u64 size, u64 entrySize);

    /* Returns the total amount of RAM in bytes. */
    u64 get_total_ram();
    /* Returns the amount of free RAM in bytes. */
    u64 get_free_ram();
    /* Returns the amount of used RAM in bytes. */
    u64 get_used_ram();

    /* Return the physical address of the base of a free
     *   page in memory, while locking it at the same time. 
     */
    void* request_page();
    /* Return the physical address of a contiguous region of physical 
     *   memory that is guaranteed to have the next `numberOfPages` 
     *   pages free, while locking all of them before returning.
     */
    void* request_pages(u64 numberOfPages);

    void lock_page(void* address);
    void lock_pages(void* address, u64 numberOfPages);

    void free_page(void* address);
    void free_pages(void* address, u64 numberOfPages);

    void print_debug();
}

#endif /* LENSOR_OS_PHYSICAL_MEMORY_MANAGER_H */
