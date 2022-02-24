#include "memory_manager.h"

#include "../bitmap.h"
#include "common.h"
#include "../cstr.h"
#include "../efi_memory.h"
#include "region.h"
#include "../uart.h"

namespace Memory {
    static bool sInitialized = false;
    static bool is_initialized() { return sInitialized; }

    static SinglyLinkedList<ManagedRegion> FreeMemory;
    static SinglyLinkedList<ManagedRegion> UsedMemory;

    static u64 sTotalFreePages;
    static u64 sTotalUsedPages;

    void init_efi(EFI_MEMORY_DESCRIPTOR* map, u64 size, u64 entrySize) {
        // Calculate number of entries within memoryMap array.
        u64 entries = size / entrySize;
        for (u64 i = 0; i < entries; ++i) {
            EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((u64)map + (i * entrySize));
            /* EFI DESCRIPTOR TYPE 7 = "EfiConventionalMemory" aka usable and free! */
            if (desc->type == 7) {
                FreeMemory.add(ManagedRegion(Region(desc->physicalAddress, desc->numPages)));
                sTotalFreePages += desc->numPages;
            }
        }
        sInitialized = true;
    }

    u64 get_free_ram() {
        if (is_initialized()) {
            return sTotalFreePages * PAGE_SIZE;
        }
        return 0;
    }

    u64 get_used_ram() {
        if (is_initialized()) {
            return sTotalUsedPages * PAGE_SIZE;
        }
        return 0;
    }

    void print_debug() {
        srl->writestr("Memory Manager Debug Dump:\r\n");
        srl->writestr("  Free Memory:\r\n");
        srl->writestr("    Total Size: ");
        srl->writestr(to_string(sTotalFreePages * PAGE_SIZE));
        srl->writestr("\r\n    Regions:");
        u64 i = 0;
        FreeMemory.for_each([&i](auto* it){
            Region* region = it->value()->region;
            srl->writestr("\r\n    Region ");
            srl->writestr(to_string(i));
            srl->writestr("\r\n      Address: 0x");
            srl->writestr(to_hexstring(region->begin()));
            srl->writestr(" thru ");
            srl->writestr(to_hexstring(region->end()));
            srl->writestr("\r\n      Page Count: ");
            srl->writestr(to_string(region->length()));
            ++i;
        });
    }
}
