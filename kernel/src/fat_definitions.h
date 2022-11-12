/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

#ifndef LENSOR_OS_FAT_DEFINITIONS_H
#define LENSOR_OS_FAT_DEFINITIONS_H

#include <integers.h>

#define FAT_DIRECTORY_SIZE_BYTES 32

// FAT File Attributes (ClusterEntry Attributes field)
#define FAT_ATTR_READ_ONLY 0b00000001
#define FAT_ATTR_HIDDEN    0b00000010
#define FAT_ATTR_SYSTEM    0b00000100
#define FAT_ATTR_VOLUME_ID 0b00001000
#define FAT_ATTR_DIRECTORY 0b00010000
#define FAT_ATTR_ARCHIVE   0b00100000
// Unused
#define FAT_ATTR_DEVICE    0b01000000
#define FAT_ATTR_UNUSED    0b10000000

// 36 BYTES
struct BIOSParameterBlock {
    /// Infinite loop to catch a computer trying to
    ///   boot from non-bootable drive: `EB FE 90`.
    u8  JumpCode[3];
    /// OEM Identifier
    u8  OEMID[8];
    u16 NumBytesPerSector;
    u8  NumSectorsPerCluster;
    /// Boot record sectors included in this value.
    u16 NumReservedSectors;
    u8  NumFATsPresent;
    u16 NumEntriesInRoot;
    /// Total sectors in logical volume.
    /// If zero, count is stored in `TotalSectors32`.
    u16 TotalSectors16;
    u8  MediaDescriptorType;
    /// FAT12/FAT16 ONLY.
    u16 NumSectorsPerFAT;
    u16 NumSectorsPerTrack;
    /// Number of heads or sides on the storage media.
    /// NOTE: Whatever program formatted the media may have been incorrect
    ///         concerning the physical geometry of the media.
    u16 NumHeadsOrSides;
    /// Number of hidden sectors (the LBA of the beginning of the partition).
    u32 NumHiddenSectors;
    u32 TotalSectors32;

    u64 first_fat_sector() {
        return NumReservedSectors;
    }

    /// Return the size of a single cluster in bytes.
    u64 cluster_size() {
        return NumSectorsPerCluster * NumBytesPerSector;
    }

    u64 root_directory_sectors() {
        return ((NumEntriesInRoot * FAT_DIRECTORY_SIZE_BYTES)
                + (NumBytesPerSector - 1)) / NumBytesPerSector;
    }

    u32 total_sectors() {
        if (TotalSectors16 == 0)
            return TotalSectors32;
        else return TotalSectors16;
    }
} __attribute__((packed));

// 26 BYTES
struct BootRecordExtension16 {
    u8  BIOSDriveNumber;
    u8  Reserved;
    u8  BootSignature;
    u32 VolumeID;
    u8  VolumeLabel[11];
    u8  FatTypeLabel[8];
} __attribute__((packed));

// 54 BYTES
struct BootRecordExtension32 {
    u32 NumSectorsPerFAT;
    u16 ExtendFlags;
    u16 FatVersion;
    u32 RootCluster;
    u16 FATInformation;
    /// Location of backup of boot record (in case of bad read/corruption).
    u16 BackupBootRecordSector;
    u8  Reserved0[12];
    u8  DriveNumber;
    u8  Reserved1;
    u8  BootSignature;
    u32 VolumeID;
    u8  VolumeLabel[11];
    u8  FatTypeLabel[8];
} __attribute__((packed));

/// Boot Record
///   Starting at logical sector zero of the partition, occupies one sector.
///   Contains both data and code mixed together for compatibility reasons.
struct BootRecord {
    // See above.
    BIOSParameterBlock BPB;
    /* This will be cast to it's specific type when the driver needs it
         based on what type of FAT the current device is (12/16 or Ex/32). */
    u8  Extended[54];
    u8  BootCode[420];
    u16 Magic; // 0xaa55

    /// Return the number of sectors per FAT.
    u64 fat_sectors() {
        if (BPB.NumSectorsPerFAT == 0)
            return ((BootRecordExtension32*)Extended)->NumSectorsPerFAT;
        else return BPB.NumSectorsPerFAT;
    }

    u64 data_sectors() {
        return BPB.total_sectors() - first_data_sector();
    }

    u64 total_clusters() {
        return data_sectors() / BPB.NumSectorsPerCluster;
    }

    u64 first_data_sector() {
        return BPB.NumReservedSectors
            + (BPB.NumFATsPresent * fat_sectors())
            + BPB.root_directory_sectors();
    }

    u64 first_root_directory_sector() {
        return first_data_sector() - BPB.root_directory_sectors();
    }

    /// Return the sector offset of a given cluster.
    u64 cluster_to_sector(u64 cluster) {
        return ((cluster - 2) * BPB.NumSectorsPerCluster) + first_data_sector();
    }

    /// Return the cluster offset of a given sector.
    u64 sector_to_cluster(u64 sector) {
        return ((sector - first_data_sector()) / BPB.NumSectorsPerCluster) + 2;
    }
} __attribute__((packed));

static_assert(sizeof(BootRecord) == 512, "Boot record must be 512 bytes.");

enum class FATType {
    INVALID = 0,
    FAT12 = 1,
    FAT16 = 2,
    FAT32 = 3,
    ExFAT = 4
};

struct ClusterEntry {
    // First 8 characters = name, last 3 = extension
    u8  FileName[11];
    /// READ_ONLY=0x01,  HIDDEN=0x02,     SYSTEM=0x04,
    /// VOLUME_ID=0x08,  DIRECTORY=0x10,  ARCHIVE=0x20,
    /// LFN=0x0f
    u8  Attributes;
    u8  Reserved0;
    u8  CTimeTenthsSecond;
    /// 5 bits for seconds, 6 bits for minutes, 5 bits for hour.
    u16 CTime;
    /// 5 bits for day, 4 bits for month, 7 bits for year.
    u16 CDate;
    u16 ADate;
    u16 ClusterNumberH;
    u16 MTime;
    u16 MDate;
    u16 ClusterNumberL;
    u32 FileSizeInBytes;

    u32 get_cluster_number() {
        u32 result = ClusterNumberL;
        result |= (u32)ClusterNumberH << 16;
        return result;
    }

    bool long_file_name() {
        return Attributes & 0b0001
            && Attributes & 0b0010
            && Attributes & 0b0100
            && Attributes & 0b1000;
    }
    bool read_only() {
        return Attributes & 0b1;
    }
    bool hidden() {
        return Attributes & 0b10;
    }
    bool system() {
        return Attributes & 0b100;
    }
    bool volume_id() {
        return Attributes & 0b1000;
    }
    bool directory() {
        return Attributes & 0b10000;
    }
    bool archive() {
        return Attributes & 0b100000;
    }

    u8 ctime_second() {
        return (CTime >> 11) & 0b11111;
    }
    u8 ctime_minute() {
        return (CTime >> 5) & 0b111111;
    }
    u8 ctime_hour() {
        return CTime & 0b11111;
    }
    u8 cdate_day() {
        return (CDate >> 11) & 0b11111;
    }
    u8 cdate_month() {
        return (CDate >> 7) & 0b1111;
    }
    u8 cdate_year() {
        return CDate & 0b1111111;
    }

    u8 adate_day() {
        return (ADate >> 11) & 0b11111;
    }
    u8 adate_month() {
        return (ADate >> 7) & 0b1111;
    }
    u8 adate_year() {
        return ADate & 0b1111111;
    }

    u8 mtime_second() {
        return (MTime >> 11) & 0b11111;
    }
    u8 mtime_minute() {
        return (MTime >> 5) & 0b111111;
    }
    u8 mtime_hour() {
        return MTime & 0b11111;
    }
    u8 mdate_day() {
        return (MDate >> 11) & 0b11111;
    }
    u8 mdate_month() {
        return (MDate >> 7) & 0b1111;
    }
    u8 mdate_year() {
        return MDate & 0b1111111;
    }

} __attribute__((packed));

/// Long File Name Cluster Entry
/// ALWAYS placed directly before their 8.3 entry (seen above).
struct LFNClusterEntry {
    u8  Order;
    /// Five two-byte characters.
    u16 Characters1[5];
    /// Always 0x0f.
    u8  Attribute;
    /// Zero for name entries.
    u8  LongEntryType;
    /// Checksum generated from short file-name when file was created.
    u8  Checksum;
    /// Six two-byte characters.
    u16 Characters2[6];
    u16 Zero;
    /// Two two-byte characters.
    u16 Characters3[2];
} __attribute__((packed));


// ExFAT
// Inspiration taken from https://github.com/dorimanx/exfat-nofuse

#define ExFAT_TYPE_UNUSED       0x0000
#define ExFAT_TYPE_DELETED      0x0001
#define ExFAT_TYPE_INVALID      0x0002
#define ExFAT_TYPE_CRITICAL_PRI 0x0100
#define ExFAT_TYPE_BITMAP       0x0101
#define ExFAT_TYPE_UPCASE       0x0102
#define ExFAT_TYPE_VOLUME       0x0103
#define ExFAT_TYPE_DIRECTORY    0x0104
#define ExFAT_TYPE_FILE         0x011f
#define ExFAT_TYPE_SYMLINK      0x015f
#define ExFAT_TYPE_CRITICAL_SEC 0x0200
#define ExFAT_TYPE_STREAM       0x0201
#define ExFAT_TYPE_EXTEND       0x0202
#define ExFAT_TYPE_ACL          0x0203
#define ExFAT_TYPE_BENIGN_PRI   0x0400
#define ExFAT_TYPE_GUID         0x0401
#define ExFAT_TYPE_PADDING      0x0402
#define ExFAT_TYPE_ACLTAB       0x0403
#define ExFAT_TYPE_BENIGN_SEC   0x0800
#define ExFAT_TYPE_ALL          0x0FFF

/// Extended File Allocation Table Boot Record
/// Occupies the first logical sector of a ExFAT formatted (floppy) disk.
/// There are some numbers that require calculation that used to be included:
///   SectorSize:  1 << SectorShift
///   ClusterSize: 1 << (SectorShift + ClusterShift)
/// Some helpful formulas:
///   ClusterArray = ClusterHeapSectorOffset * SectorSize - 2 * ClusterSize
///   FAToffset    = FATsectorOffset * SectorSize
///   Usable Space = NumClusters * ClusterSize
struct ExFATBootRecord {
    u8  JumpCode[3] { 0xeb, 0xfe, 0x90 };
    u8  OEMID[8];
    u8  Reserved0[53];
    u64 PartitionOffset;
    u64 VolumeLength;
    u32 FATsectorOffset;
    u32 NumFATsectors;
    u32 ClusterHeapSectorOffset;
    u32 NumClusters;
    u32 RootDirectoryCluster;
    u32 PartitionSerialNumber;
    u16 FilesystemRevision;
    u16 Flags;
    u8  SectorShift;
    u8  ClusterShift;
    u8  NumFATsPresent;
    u8  PhysicalDriveNumber;
    u8  PercentageInUse;
    u8  Reserved1[7];
}  __attribute__((packed));

struct ExFATFileEntry {
    u8  Type;
    u8  NumSecondaries;
    u16 Checksum;
    u16 Attributes;
    u16 Reserved0;
    u16 CTime;
    u16 CDate;
    u16 MTime;
    u16 MDate;
    u16 ATime;
    u16 ADate;
    u8  CMilliseconds;
    u8  MMilliseconds;
    u8  CTimeUTCOffset;
    u8  MTimeUTCOffset;
    u8  ATimeUTCOffset;
    u8  Reserved1[7];
} __attribute__((packed));

struct ExFATStreamExtensionEntry {
    u8  Type;
    u8  SecondaryFlags;
    u8  Reserved0;
    u8  NameLength;
    u16 NameHash;
    u16 Reserved1;
    u64 ValidDataLength;
    u32 Reserved2;
    u32 FirstCluster;
    u64 DataLength;
} __attribute__((packed));

struct ExFATFileNameEntry {
    u8 Type;
    u8 Flags;
    u8 FileName[30];
} __attribute__((packed));

#endif
