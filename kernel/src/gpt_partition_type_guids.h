#ifndef LENSOR_OS_GPT_PARTITION_TYPE_GUIDS_H
#define LENSOR_OS_GPT_PARTITION_TYPE_GUIDS_H

#include "guid.h"

namespace GPT {
    /* GPT GUIDs
     * |-- Unused                  -> 00000000-0000-0000-0000-000000000000
     * |-- MBR                     -> 024DEE41-33E7-11D3-9D69-0008C781F39F
     * |-- EFI System              -> C12A7328-F81F-11D2-BA4B-00A0C93EC93B
     * |-- BIOS Boot               -> 21686148-6449-6E6F-744E-656564454649
     * `-- Intel Fast Flash (iFFS) -> D3BFE2DE-3DAF-11DF-BA40-E3A556D89593
     */
    constexpr GUID PartitionType$Unused = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};
    constexpr GUID PartitionType$MBR =
        { 0x024dee41, 0x33e7, 0x11d3, { 0x9d, 0x69, 0x00, 0x08, 0xc7, 0x81, 0xf3, 0x9f } };
    constexpr GUID PartitionType$EFISystem =
        { 0xc12a7328, 0xf81f, 0x11d2, { 0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b } };
    constexpr GUID PartitionType$BIOSBoot =
        { 0x21686148, 0x6449, 0x6e6f, { 0x74, 0x4e, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49 } };
    constexpr GUID PartitionType$IntelFastFlash =
        { 0xd3bfe2de, 0x3daf, 0x11df, { 0xba, 0x40, 0xe3, 0xa5, 0x56, 0xd8, 0x96, 0x93 } };

    /* Microsoft Windows:
     * |-- Reserved                      -> E3C9E316-0B5C-4DB8-817D-F92DF00215AE
     * |-- Basic Data                    -> EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
     * |-- Logical Disk Manager Metadata -> 5808C8AA-7E8F-42E0-85D2-E1E90434CFB3
     * |-- Logical Disk Manager Data     -> AF9B60A0-1431-4F62-BC68-3311714A69AD
     * |-- Recovery                      -> DE94BBA4-06D1-4D40-A16A-BFD50179D6AC
     * `-- IBM GPFS                      -> 37AFFC90-EF7D-4E96-91C3-2D7AE055B174
     */
    constexpr GUID PartitionType$MSWindows_Reserved =
        { 0xe3c9e316, 0x0b5c, 0x4db8, { 0x81, 0x7d, 0xf9, 0x2d, 0xf0, 0x02, 0x15, 0xae } };
    constexpr GUID PartitionType$MSWindows_BasicData =
        { 0xebd0a0a2, 0xb9e5, 0x4433, { 0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7 } };
    constexpr GUID PartitionType$MSWindows_LogicalDiskManagerMetadata =
        { 0x5808c8aa, 0x7e8f, 0x42e0, { 0x85, 0xd2, 0xe1, 0xe9, 0x04, 0x34, 0xcf, 0xb3 } };
    constexpr GUID PartitionType$MSWindows_LogicalDiskManagerData =
        { 0xaf9b60a0, 0x1431, 0x4f62, { 0xbc, 0x68, 0x33, 0x11, 0x71, 0x4a, 0x69, 0xad } };
    constexpr GUID PartitionType$MSWindows_Recovery =
        { 0xde94bba4, 0x06d1, 0x4d40, { 0xa1, 0x6a, 0xbf, 0xd5, 0x01, 0x79, 0xd6, 0xac } };
    constexpr GUID PartitionType$MSWindows_IBMGeneralParallelFileSystem =
        { 0x37affc90, 0xef7d, 0x4e96, { 0x91, 0xc3, 0x2d, 0x7a, 0xe0, 0x55, 0xb1, 0x74 } };

    /* Linux
     * |-- File System Data       -> 0FC63DAF-8483-4772-8E79-3D69D8477DE4
     * |-- RAID                   -> A19D880F-05FC-4D3B-A006-743F0F84911E
     * |-- Swap                   -> 0657FD6D-A4AB-43C4-84E5-0933C84B4F4F
     * |-- Logical Volume Manager -> E6D6D379-F507-44C2-A23C-238F2A3DF928
     * `-- Reserved               -> 8DA63339-0007-60C0-C436-083AC8230908
     */
    constexpr GUID PartitionType$Linux_FileSystemData =
        { 0x0fc63daf, 0x8483, 0x4772, { 0x8e, 0x79, 0x3d, 0x69, 0xd8, 0x47, 0x7d, 0xe4 } };
    constexpr GUID PartitionType$Linux_RAID =
        { 0xa19d880f, 0x05fc, 0x4d3b, { 0xa0, 0x06, 0x74, 0x3f, 0x0f, 0x84, 0x91, 0x1e } };
    constexpr GUID PartitionType$Linux_Swap =
        { 0x0657fd6d, 0xa4ab, 0x43c4, { 0x84, 0xe5, 0x09, 0x33, 0xc8, 0x4b, 0x4f, 0x4f } };
    constexpr GUID PartitionType$Linux_LogicalVolumeManager =
        { 0xe6d6d379, 0xf507, 0x44c2, { 0xa2, 0x3c, 0x23, 0x8f, 0x2a, 0x3d, 0xf9, 0x28 } };
    constexpr GUID PartitionType$Linux_Reserved =
        { 0x8da63339, 0x0007, 0x60c0, { 0xc4, 0x36, 0x08, 0x3a, 0xc8, 0x23, 0x09, 0x08 } };

    /* FreeBSD
     * |-- Boot                 -> 83BD6B9D-7F41-11DC-BE0B-001560B84F0F
     * |-- Data                 -> 516E7CB4-6ECF-11D6-8FF8-00022D09712B
     * |-- Swap                 -> 516E7CB5-6ECF-11D6-8FF8-00022D09712B
     * |-- UFS                  -> 516E7CB6-6ECF-11D6-8FF8-00022D09712B
     * |-- Vinum Volume Manager -> 516E7CB8-6ECF-11D6-8FF8-00022D09712B
     * `-- ZFS                  -> 516E7CBA-6ECF-11D6-8FF8-00022D09712B
     */
    constexpr GUID PartitionType$FreeBSD_Boot =
        { 0x83bd6b9d, 0x7f41, 0x11dc, {0xbe, 0x0b, 0x00, 0x15, 0x60, 0xb8, 0x4f, 0x0f } };
    constexpr GUID PartitionType$FreeBSD_Data =
        { 0x516e7cb4, 0x6ecf, 0x11d6, {0x8f, 0xf8, 0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b } };
    constexpr GUID PartitionType$FreeBSD_Swap =
        { 0x516e7cb5, 0x6ecf, 0x11d6, {0x8f, 0xf8, 0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b } };
    constexpr GUID PartitionType$FreeBSD_UFS =
        { 0x516e7cb6, 0x6ecf, 0x11d6, {0x8f, 0xf8, 0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b } };
    constexpr GUID PartitionType$FreeBSD_VinumVolumeManager =
        { 0x516e7cb8, 0x6ecf, 0x11d6, {0x8f, 0xf8, 0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b } };
    constexpr GUID PartitionType$FreeBSD_ZFS =
        { 0x516e7cba, 0x6ecf, 0x11d6, {0x8f, 0xf8, 0x00, 0x02, 0x2d, 0x09, 0x71, 0x2b } };

    /* NetBSD 
     * |--  Swap         -> 49F48D32-B10E-11DC-B99B-0019D1879648
     * |--  FFS          -> 49F48D5A-B10E-11DC-B99B-0019D1879648
     * |--  LFS          -> 49F48D82-B10E-11DC-B99B-0019D1879648
     * |--  RAID         -> 49F48DAA-B10E-11DC-B99B-0019D1879648
     * |--  Concatenated -> 2DB519C4-B10F-11DC-B99B-0019D1879648
     * `--  Encrypted    -> 2DB519EC-B10F-11DC-B99B-0019D1879648
     */
    constexpr GUID PartitionType$NetBSD_Swap =
        { 0x49f48d32, 0xb10e, 0x11dc, { 0xb9, 0x9b, 0x00, 0x19, 0xd1, 0x87, 0x96, 0x48 } };
    constexpr GUID PartitionType$NetBSD_FFS =
        { 0x49f48d5a, 0xb10e, 0x11dc, { 0xb9, 0x9b, 0x00, 0x19, 0xd1, 0x87, 0x96, 0x48 } };
    constexpr GUID PartitionType$NetBSD_LFS =
        { 0x49f48d82, 0xb10e, 0x11dc, { 0xb9, 0x9b, 0x00, 0x19, 0xd1, 0x87, 0x96, 0x48 } };
    constexpr GUID PartitionType$NetBSD_RAID =
        { 0x49f48daa, 0xb10e, 0x11dc, { 0xb9, 0x9b, 0x00, 0x19, 0xd1, 0x87, 0x96, 0x48 } };
    constexpr GUID PartitionType$NetBSD_Concatenated =
        { 0x2db519c4, 0xb10e, 0x11dc, { 0xb9, 0x9b, 0x00, 0x19, 0xd1, 0x87, 0x96, 0x48 } };
    constexpr GUID PartitionType$NetBSD_Encrypted =
        { 0x2db519ec, 0xb10e, 0x11dc, { 0xb9, 0x9b, 0x00, 0x19, 0xd1, 0x87, 0x96, 0x48 } };

    /* MidnightBSD
     * |-- Data                   -> 85D5E45A-237C-11E1-B4B3-E89A8F7FC3A7
     * |-- Swap                   -> 85D5E45B-237C-11E1-B4B3-E89A8F7FC3A7
     * |-- Vinum volume manager   -> 85D5E45C-237C-11E1-B4B3-E89A8F7FC3A7
     * |-- ZFS                    -> 85D5E45D-237C-11E1-B4B3-E89A8F7FC3A7
     * |-- Boot                   -> 85D5E45E-237C-11E1-B4B3-E89A8F7FC3A7
     * `-- Unix File System (UFS) -> 0394EF8B-237E-11E1-B4B3-E89A8F7FC3A7
     */
    constexpr GUID PartitionType$MidnightBSD_Data =
        { 0x85d5e45a, 0x237c, 0x11e1, { 0xb4, 0xb3, 0xe8, 0x9a, 0x8f, 0x7f, 0xc3, 0xa7 } };
    constexpr GUID PartitionType$MidnightBSD_Swap =
        { 0x85d5e45b, 0x237c, 0x11e1, { 0xb4, 0xb3, 0xe8, 0x9a, 0x8f, 0x7f, 0xc3, 0xa7 } };
    constexpr GUID PartitionType$MidnightBSD_VinumVolumeManager =
        { 0x85d5e45c, 0x237c, 0x11e1, { 0xb4, 0xb3, 0xe8, 0x9a, 0x8f, 0x7f, 0xc3, 0xa7 } };
    constexpr GUID PartitionType$MidnightBSD_ZFS =
        { 0x85d5e45d, 0x237c, 0x11e1, { 0xb4, 0xb3, 0xe8, 0x9a, 0x8f, 0x7f, 0xc3, 0xa7 } };
    constexpr GUID PartitionType$MidnightBSD_Boot =
        { 0x85d5e45e, 0x237c, 0x11e1, { 0xb4, 0xb3, 0xe8, 0x9a, 0x8f, 0x7f, 0xc3, 0xa7 } };
    constexpr GUID PartitionType$MidnightBSD_UFS =
        { 0x0394ef8b, 0x237e, 0x11e1, { 0xb4, 0xb3, 0xe8, 0x9a, 0x8f, 0x7f, 0xc3, 0xa7 } };
    
    /* ChromeOS 
     * |-- Kernel     -> FE3A2A5D-4F32-41A7-B725-ACCC3285A309
     * |-- RootFS     -> 3CB8E202-3B7E-47DD-8A3C-7FF2A13CFCEC
     * `-- Future Use -> 2E0A753D-9E48-43B0-8337-B15192CB1B5E
     */
    constexpr GUID PartitionType$ChromeOS_Kernel =
        { 0xfe3a2a5d, 0x4f32, 0x41a7, { 0xb7, 0x25, 0xac, 0xcc, 0x32, 0x85, 0xa3, 0x09 } };
    constexpr GUID PartitionType$ChromeOS_RootFS =
        { 0x3cb8e202, 0x3b7e, 0x47dd, { 0x8a, 0x3c, 0x7f, 0xf2, 0xa1, 0x3c, 0xfc, 0xec } };
    constexpr GUID PartitionType$ChromeOS_FutureUse =
        { 0x2e0a753d, 0x9e48, 0x43b0, { 0x83, 0x37, 0xb1, 0x51, 0x92, 0xcb, 0x1b, 0x5e } };

    /* HP-UX 
     * |-- Data    -> 75894C1E-3AEB-11D3-B7C1-7B03A0000000
     * `-- Service -> E2A1E728-32E3-11D6-A682-7B03A0000000
     */
    constexpr GUID PartitionType$HPUX_Data =
        { 0x75894c1e, 0x3aeb, 0x11d3, { 0xb7, 0xc1, 0x7b, 0x03, 0xa0, 0x00, 0x00, 0x00 } };
    constexpr GUID PartitionType$HPUX_Service =
        { 0xe2a1e728, 0x32e3, 0x11d6, { 0xa6, 0x82, 0x7b, 0x03, 0xa0, 0x00, 0x00, 0x00 } };

    /* Solaris
     * |-- Boot partition    -> 6A82CB45-1DD2-11B2-99A6-080020736631
     * |-- Root partition    -> 6A85CF4D-1DD2-11B2-99A6-080020736631
     * |-- Swap partition    -> 6A87C46F-1DD2-11B2-99A6-080020736631
     * |-- Backup partition  -> 6A8B642B-1DD2-11B2-99A6-080020736631
     * |-- `/usr` partition  -> 6A898CC3-1DD2-11B2-99A6-080020736631
     * |-- `/var` partition  -> 6A8EF2E9-1DD2-11B2-99A6-080020736631
     * |-- `/home` partition -> 6A90BA39-1DD2-11B2-99A6-080020736631
     * |-- Alternate sector  -> 6A9283A5-1DD2-11B2-99A6-080020736631
     * `-- Reserved:
     *     |-- 6A945A3B-1DD2-11B2-99A6-080020736631
     *     |-- 6A9630D1-1DD2-11B2-99A6-080020736631
     *     |-- 6A980767-1DD2-11B2-99A6-080020736631
     *     |-- 6A96237F-1DD2-11B2-99A6-080020736631
     *     `-- 6A8D2AC7-1DD2-11B2-99A6-080020736631
     */
    constexpr GUID PartitionType$Solaris_Boot =
        { 0x6a82cb45, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_Root =
        { 0x6a85cf4d, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_Swap =
        { 0x6a87c46f, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_Backup =
        { 0x6a8b642b, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_usr =
        { 0x6a898cc3, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_var =
        { 0x6a8ef2e9, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_home =
        { 0x6a90ba39, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_AlternateSector =
        { 0x6a9283a5, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_Reserved1 =
        { 0x6a945a3b, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_Reserved2 =
        { 0x6a9630d1, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_Reserved3 =
        { 0x6a980767, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_Reserved4 =
        { 0x6a96237f, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
    constexpr GUID PartitionType$Solaris_Reserved5 =
        { 0x6a8d2ac7, 0x1dd2, 0x11b2, { 0x99, 0xa6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31 } };
}

#endif /* LENSOR_OS_GPT_PARTITION_TYPE_GUIDS_H */
