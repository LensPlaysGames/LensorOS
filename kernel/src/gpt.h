#ifndef LENSOR_OS_GPT_H
#define LENSOR_OS_GPT_H

#include <ahci.h>
#include <guid.h>
#include <integers.h>
#include <system.h>
#include <gpt_partition_type_guids.h>

namespace GPT {
    // If a partition's GUID appears in this list, the
    // partition will be left untouched by LensorOS.
    constexpr GUID ReservedPartitionGUIDs[] = {
        /* Universal */
        PartitionType$MBR,
        PartitionType$BIOSBoot,
        PartitionType$IntelFastFlash,

        /* Windows */
        PartitionType$MSWindows_Reserved,
        PartitionType$MSWindows_BasicData,
        PartitionType$MSWindows_LogicalDiskManagerMetadata,
        PartitionType$MSWindows_LogicalDiskManagerData,
        PartitionType$MSWindows_Recovery,
        PartitionType$MSWindows_IBMGeneralParallelFileSystem,

        /* Linux */
        PartitionType$Linux_FileSystemData,
        PartitionType$Linux_RAID,
        PartitionType$Linux_Swap,
        PartitionType$Linux_LogicalVolumeManager,
        PartitionType$Linux_Reserved,

        /* FreeBSD */
        PartitionType$FreeBSD_Boot,
        PartitionType$FreeBSD_Data,
        PartitionType$FreeBSD_Swap,
        PartitionType$FreeBSD_UFS,
        PartitionType$FreeBSD_VinumVolumeManager,
        PartitionType$FreeBSD_ZFS,

        /* NetBSD */
        PartitionType$NetBSD_Swap,
        PartitionType$NetBSD_FFS,
        PartitionType$NetBSD_LFS,
        PartitionType$NetBSD_RAID,
        PartitionType$NetBSD_Concatenated,
        PartitionType$NetBSD_Encrypted,

        /* MidnightBSD */
        PartitionType$MidnightBSD_Data,
        PartitionType$MidnightBSD_Swap,
        PartitionType$MidnightBSD_VinumVolumeManager,
        PartitionType$MidnightBSD_ZFS,
        PartitionType$MidnightBSD_Boot,
        PartitionType$MidnightBSD_UFS,

        /* ChromeOS */
        PartitionType$ChromeOS_Kernel,
        PartitionType$ChromeOS_RootFS,
        PartitionType$ChromeOS_FutureUse,

        /* HP-UX */
        PartitionType$HPUX_Data,
        PartitionType$HPUX_Service,

        /* Solaris */
        PartitionType$Solaris_Boot,
        PartitionType$Solaris_Root,
        PartitionType$Solaris_Swap,
        PartitionType$Solaris_Backup,
        PartitionType$Solaris_usr,
        PartitionType$Solaris_var,
        PartitionType$Solaris_home,
        PartitionType$Solaris_AlternateSector,
        PartitionType$Solaris_Reserved1,
        PartitionType$Solaris_Reserved2,
        PartitionType$Solaris_Reserved3,
        PartitionType$Solaris_Reserved4,
        PartitionType$Solaris_Reserved5,
    };

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
    enum PartitionAttributesBits {
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
        GUID TypeGUID;
        GUID UniqueGUID;
        u64 StartLBA;
        u64 EndLBA;
        u64 Attributes;
        // FIXME: Should be 36 UTF-16 code points.
        u8 Name[72];

        inline u64 size_in_sectors() { return EndLBA - StartLBA; }

        /* Attributes  */
        inline bool required_by_platform() { return Attributes & (1 << PartitionAttributesBits::PlatformRequired); }
        inline bool should_ignore() { return Attributes & (1 << PartitionAttributesBits::Ignore); }
    } __attribute__((packed));;

    bool is_gpt_present(StorageDeviceDriver* driver);
}

#endif /* LENSOR_OS_GPT_H */
