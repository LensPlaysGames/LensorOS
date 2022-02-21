#include "acpi.h"

#include "cstr.h"
#include "integers.h"
#include "uart.h"

/* Helpful resource: https://github.com/freebsd/freebsd-src/blob/main/usr.sbin/acpi/acpidump/acpi.c */

namespace ACPI {
    u8 checksum(void* pointer, u64 length) {
        u8* base = (u8*)pointer;
        u8 sum = 0;
        for (; length > 0; length--, base++)
            sum += *base;

        return sum;
    }
    
    void* find_table(SDTHeader* header, char* signature) {
        u64 entries = (header->Length - sizeof(ACPI::SDTHeader)) / 8;
        for (u64 t = 0; t < entries; ++t) {
            SDTHeader* sdt = (SDTHeader*)*((u64*)((u64)header + sizeof(SDTHeader)) + t);
            // Find matching signature.
            if (strcmp((char*)sdt->Signature, signature, 4)) {
                if (int rc = checksum(sdt, sdt->Length)) {
                    srl->writestr("[ACPI]: ERROR -> Invalid checksum on '");
                    srl->writestr((char*)sdt->Signature, 4);
                    srl->writestr("' table: ");
                    srl->writestr(to_string(rc));
                    srl->writestr("\r\n");
                    return nullptr;
                }
                return sdt;
            }
        }
        // Could not find table.
        return nullptr;
    }
}
