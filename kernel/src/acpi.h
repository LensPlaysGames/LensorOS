#ifndef LENSOR_OS_ACPI_H
#define LENSOR_OS_ACPI_H

#include "integers.h"

namespace ACPI {
    struct RSDP2 {
        unsigned char Signature[8];
        u8 Checksum;
        u8 OEMId[6];
        u8 Revision;
        u32 RSDTAddress;
        u32 Length;
        u64 XSDTAddress;
        u8 ExtendedChecksum;
        u8 Reserved[3];
    } __attribute__((packed));

    struct SDTHeader {
        unsigned char Signature[4];
        u32 Length;
        u8 Revision;
        u8 Checksum;
        u8 OEMID[6];
        u8 OEMTableID[8];
        u32 OEMRevision;
        u32 CreatorID;
        u32 CreatorRevision;
    } __attribute__((packed));

    struct MCFGHeader {
        SDTHeader Header;
        u64 Reserved;
    } __attribute__((packed));

    struct DeviceConfig {
        u64 BaseAddress;
        u16 PCISegmentGroup;
        u8 StartBus;
        u8 EndBus;
        u32 Reserved;
    } __attribute__((packed));

    void* find_table(SDTHeader* sdt, char* signature);
}

#endif
