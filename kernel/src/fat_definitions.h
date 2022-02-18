#ifndef LENSOR_OS_FAT_DEFINITIONS_H
#define LENSOR_OS_FAT_DEFINITIONS_H

#include "integers.h"

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

    
} __attribute__((packed));

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
