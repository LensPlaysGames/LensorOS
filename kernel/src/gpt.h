#ifndef LENSOR_OS_GPT_H
#define LENSOR_OS_GPT_H

#include "ahci.h"

namespace GPT {
    struct Header {
        // Signature should be "EFI PART"
        unsigned char Signature[8];
        u32 Revision { 0 };
        u32 Size { 0 };
        u32 CRC32 { 0 };
        u32 Reserved0 { 0 };
        u64 CurrentLBA { 0 };
        u64 BackupLBA { 0 };
        u64 FirstUsableLBA { 0 };
        u64 LastUsableLBA { 0 };
        u8 DiskGUID[16];
        u64 PartitionsTableLBA { 0 };
        u32 NumberOfPartitionsTableEntries { 0 };
        u32 PartitionsTableEntrySize { 0 };
    } __attribute__((packed));

    // Shift a 1 to the left by this many bits to get to the
    // corresponding flag bit in PartitionEntry Attributes.
    enum PartionAttributesBits {
        PlatformRequired = 0,
        Ignore = 1,
        LegacyBIOSBootable = 2,
    };

    enum BasicDataPartitionTypeSpecificAttributesBits {
        ReadOnly = 60,
        ShadowCopy = 61,
        Hidden = 62,
        NoDriveLetter = 63,
    };

    struct PartitionEntry {
        u8 TypeGUID[16];
        u8 UniqueGUID[16];
        u64 StartLBA;
        u64 EndLBA;
        u64 Attributes;
        // FIXME: Should be 36 UTF-16 code points, not 72 raw bytes.
        u8 Name[72];
    } __attribute__((packed));;

    bool is_gpt_present(AHCI::Port*);
}

#endif /* LENSOR_OS_GPT_H */
