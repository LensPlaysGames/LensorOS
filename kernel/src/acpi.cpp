#include <acpi.h>

#include <cstr.h>
#include <debug.h>
#include <integers.h>

/* Helpful resource: https://github.com/freebsd/freebsd-src/blob/main/usr.sbin/acpi/acpidump/acpi.c */

// Uncomment the following directive for extra debug information output.
#define DEBUG_ACPI


namespace ACPI {
    void initialize(RSDP2* rootSystemDescriptorPointer) {
#ifdef DEBUG_ACPI
        dbgmsg("[ACPI]: Initializing ACPI\r\n");
#endif /* DEBUG_ACPI */
        if (rootSystemDescriptorPointer == nullptr) {
            dbgmsg("[ACPI]: \033[31mERROR\033[0m -> "
                   "Root System Descriptor Pointer is null. "
                   "(error in bootloader or during boot process)\r\n"
                   );
            return;
        }
        gRSDP = (ACPI::SDTHeader*)rootSystemDescriptorPointer;
        // eXtended System Descriptor Table
        gXSDT = (ACPI::SDTHeader*)(rootSystemDescriptorPointer->XSDTAddress);
#ifdef DEBUG_ACPI
        dbgmsg("  RSDP %x\r\n"
               "  XSDT: %x\r\n"
               "[ACPI]: \033[32mInitialized\033[0m\r\n"
               "\r\n"
               , gRSDP
               , gXSDT
               );
#endif /* DEBUG_ACPI */
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

        dbgmsg("Signature: ");
        dbgmsg(header->Signature, 4, ShouldNewline::Yes);
        dbgmsg("  Length: %ul\r\n"
               "  Revision: %hhu\r\n"
               "  Checksum: %hhu\r\n"
               , header->Length
               , header->Revision
               , header->Checksum
               );
        dbgmsg("  OEM ID: ");
        dbgmsg(header->OEMID, 6, ShouldNewline::Yes);
        dbgmsg("  OEM Table ID: ");
        dbgmsg(header->OEMTableID, 8, ShouldNewline::Yes);
        dbgmsg("  OEM Revision: %ul\r\n", header->OEMRevision);
        dbgmsg("  Creator ID: ");
        dbgmsg((u8*)&header->CreatorID, 4, ShouldNewline::Yes);
        dbgmsg("  Creator Revision: %ul\r\n", header->CreatorRevision);
    }

    void* find_table(SDTHeader* header, const char* signature) {
        if (header == nullptr || signature == nullptr)
            return nullptr;

#ifdef DEBUG_ACPI
        dbgmsg("[ACPI]: Looking for %s table\r\n", signature);
#endif /* DEBUG_ACPI */

        u64 entries = (header->Length - sizeof(ACPI::SDTHeader)) / 8;

#ifdef DEBUG_ACPI
        dbgmsg("  ");
        dbgmsg(header->Signature, 4);
        dbgmsg(": %ull entries\r\n", entries);
#endif /* DEBUG_ACPI */

        for (u64 t = 0; t < entries; ++t) {
            SDTHeader* sdt = (SDTHeader*)*((u64*)((u64)header + sizeof(SDTHeader)) + t);
#ifdef DEBUG_ACPI
            print_sdt(sdt);
#endif /* DEBUG_ACPI */
            // Find matching signature.
            if (strcmp((char*)sdt->Signature, signature, 4)) {
                if (int rc = checksum(sdt, sdt->Length)) {
                    dbgmsg("[ACPI]: \033[31mERROR::\033[0m Invalid checksum on '");
                    dbgmsg(sdt->Signature, 4);
                    dbgmsg("' table: %i\r\n"
                           "\r\n", rc);
                    return nullptr;
                }
#ifdef DEBUG_ACPI
                dbgmsg("\r\n");
#endif /* DEBUG_ACPI */
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
