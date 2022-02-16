#include "acpi.h"

namespace ACPI {
    void* find_table(SDTHeader* header, char* signature) {
        u64 entries = (header->Length - sizeof(ACPI::SDTHeader)) / 8;
        for (u64 t = 0; t < entries; ++t) {
            ACPI::SDTHeader* sdt = (ACPI::SDTHeader*)*(u64*)((u64)header + sizeof(ACPI::SDTHeader) + (t * 8));
            for (u8 i = 0; i < 4; ++i) {
                if (sdt->Signature[i] != signature[i])
                    break;

                if (i == 3)
                    return sdt;
            }
        }
        // Could not find table.
        return 0;
    }
}
