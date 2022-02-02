#ifndef LENSOR_OS_FAT_DEFINITIONS_H
#define LENSOR_OS_FAT_DEFINITIONS_H

#include "integers.h"

#define FAT_DIRECTORY_SIZE_BYTES 32

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

struct BootRecordExtension16 {
	u8  BIOSDriveNumber;
	u8  Reserved;
	u8  BootSignature;
	u32 VolumeID;
	u8  VolumeLabel[11];
	u8  FatTypeLabel[8];
} __attribute__((packed));

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
	u8 Extended[54];
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
	///	LFN=0x0f
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

// TODO: ExFAT directory entry struct.

#endif
