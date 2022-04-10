#include <acpi.h>

#include <cstr.h>
#include <integers.h>
#include <uart.h>

/* Helpful resource: https://github.com/freebsd/freebsd-src/blob/main/usr.sbin/acpi/acpidump/acpi.c */

namespace ACPI {
    void initialize(RSDP2* rootSystemDescriptorPointer) {
        UART::out("[ACPI]: Initializing ACPI\r\n"
                  "  RSDP: 0x");
        UART::out(to_hexstring(rootSystemDescriptorPointer));
        UART::out("\r\n");
        if (rootSystemDescriptorPointer == nullptr) {
            UART::out("[ACPI]: \033[31mERROR\033[0m -> Root System Descriptor Pointer is null. "
                      "(error in bootloader or during boot process)\r\n");
            return;
        }
        gRSDP = (ACPI::SDTHeader*)rootSystemDescriptorPointer;
        // eXtended System Descriptor Table
        gXSDT = (ACPI::SDTHeader*)(rootSystemDescriptorPointer->XSDTAddress);
        UART::out("  XSDT: 0x");
        UART::out(to_hexstring(gXSDT));
        UART::out("\r\n"
                  "[ACPI]: \033[32mInitialized\033[0m\r\n");
    }

    SDTHeader* gRSDP { nullptr };
    SDTHeader* gXSDT { nullptr };

    u8 checksum(void* pointer, u64 length) {
        u8* base = (u8*)pointer;
        u8 sum = 0;
        for (; length > 0; length--, base++)
            sum += *base;

        return sum;
    }

    void print_sdt(SDTHeader* header) {
        if (header == nullptr)
            return;

        UART::out("Signature: ");
        UART::out(header->Signature, 4);
        UART::out("\r\n"
                  "  Length: ");
        UART::out(header->Length);
        UART::out("\r\n"
                  "  Revision: ");
        UART::out(to_string(header->Revision));
        UART::out("\r\n"
                  "  Checksum: ");
        UART::out(to_string(header->Checksum));
        UART::out("\r\n"
                  "  OEM ID: ");
        UART::out(header->OEMID, 6);
        UART::out("\r\n"
                  "  OEM Table ID: ");
        UART::out(header->OEMTableID, 8);
        UART::out("\r\n"
                  "  OEM Revision: ");
        UART::out(header->OEMRevision);
        UART::out("\r\n"
                  "  Creator ID: ");
        UART::out((u8*)&header->CreatorID, 4);
        UART::out("\r\n"
                  "  Creator Revision: ");
        UART::out(header->CreatorRevision);
        UART::out("\r\n");
    }

    void* find_table(SDTHeader* header, const char* signature) {
        if (header == nullptr || signature == nullptr)
            return nullptr;

        UART::out("[ACPI]: Looking for ");
        UART::out(signature);
        UART::out(" table\r\n");

        u64 entries = (header->Length - sizeof(ACPI::SDTHeader)) / 8;

        UART::out("  ");
        UART::out(header->Signature, 4);
        UART::out(": ");
        UART::out(entries);
        UART::out(" entries\r\n");

        for (u64 t = 0; t < entries; ++t) {
            SDTHeader* sdt = (SDTHeader*)*((u64*)((u64)header + sizeof(SDTHeader)) + t);
            print_sdt(sdt);
            // Find matching signature.
            if (strcmp((char*)sdt->Signature, signature, 4)) {
                if (int rc = checksum(sdt, sdt->Length)) {
                    UART::out("[ACPI]: \033[31mERROR::\033[0m Invalid checksum on '");
                    UART::out(sdt->Signature, 4);
                    UART::out("' table: ");
                    UART::out(to_string(rc));
                    UART::out("\r\n");
                    return nullptr;
                }
                return sdt;
            }
        }
        // Could not find table.
        return nullptr;
    }

    void* find_table(const char* signature) {
        return find_table(gXSDT, signature);
    }
}
