#include "acpi.h"

#include "cstr.h"

namespace ACPI {
    void* find_table(SDTHeader* header, char* signature) {
        u64 entries = (header->Length - sizeof(ACPI::SDTHeader)) / 8;
        for (u64 t = 0; t < entries; ++t) {
            ACPI::SDTHeader* sdt = (ACPI::SDTHeader*)*(u64*)((u64)header + sizeof(ACPI::SDTHeader) + (t * 8));
            if (strcmp((char*)sdt->Signature, signature, 4))
                return sdt;
        }
        // Could not find table.
        return nullptr;
    }
}
