#include "acpi.h"

#include "cstr.h"
#include "integers.h"
#include "uart.h"

/* Helpful resource: https://github.com/freebsd/freebsd-src/blob/main/usr.sbin/acpi/acpidump/acpi.c */

namespace ACPI {
    void initialize(RSDP2* rootSystemDescriptorPointer) {
        if (rootSystemDescriptorPointer == nullptr) {
            srl->writestr("[ACPI]: \033[31mERROR\033[0m -> Root System Descriptor Pointer is null. ");
            srl->writestr("(most likely error in bootloader/during boot process)\r\n");
            return;
        }
        // eXtended System Descriptor Table
        gXSDT = (ACPI::SDTHeader*)(rootSystemDescriptorPointer->XSDTAddress);
    }

    SDTHeader* gXSDT { nullptr };
    
    u8 checksum(void* pointer, u64 length) {
        u8* base = (u8*)pointer;
        u8 sum = 0;
        for (; length > 0; length--, base++)
            sum += *base;

        return sum;
    }
    
    void* find_table(SDTHeader* header, char* signature) {
        if (header == nullptr || signature == nullptr)
            return nullptr;
        
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
    
    void* find_table(char* signature) {
        return find_table(gXSDT, signature);
    }

    void* find_table(const char* signature) {
        return find_table((char*)signature);
    }
}
