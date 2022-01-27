#ifndef LENSOR_OS_FILE_SYSTEM_FAT_H
#define LENSOR_OS_FILE_SYSTEM_FAT_H

#include "../integers.h"
#include "../ahci.h"
#include "../basic_renderer.h"

#define FAT_DIRECTORY_SIZE_BYTES 32

// Resource Used: https://wiki.osdev.org/FAT#Programming_Guide

// Thanks to Gigasoft of osdev forums for this list
// What makes a FAT filesystem valid:
// - Word at 0x1fe equates to 0xaa55
// - Sector size is power of two between 512-4096 (inclusive)
// - Cluster size of a power of two
// - Media type is 0xf0 or greater or equal to 0xf8
// - FAT size is not zero
// - Number of sectors is not zero
// - Number of root directory entries is (zero if fat32) (not zero if fat12/16)
// - Root cluster is valid (FAT32)
// - File system version is zero (FAT32)
// - NumFATsPresent greater than zero

/// File Allocation Table File System
///   Formats storage media into three sections:
///     - Boot Record
///     - File Allocation Table (namesake)
///     - Directory + Data area (they couldn't name
///         this something cool like the other two?)
namespace FatFS {
	/// BIOSParameterBlock
	///   Initial section of first logical sector on storage media.
	///   Contains information such as number of bytes per sector,
	///     num sectors per cluster, num reserved sectors, etc.
	struct BIOSParameterBlock {
		/// Infinite loop to catch a computer trying to
		///   boot from non-bootable drive: `EB FE 90`.
		u8 JumpCode[3];
		/// OEM Identifier
		u8 OEMID[8];
		u16 NumBytesPerSector;
		u8 NumSectorsPerCluster;
		/// Boot record sectors included in this value.
		u16 NumReservedSectors;
		u8 NumFATsPresent;
		u16 NumEntriesInRoot;
		/// Total sectors in logical volume.
		/// If zero, count is stored in `TotalSectors32`.
		u16 TotalSectors16;
		u8 MediaDescriptorType;
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
		u8 BIOSDriveNumber;
		u8 Reserved;
		u8 BootSignature;
		u32 VolumeID;
		u8 VolumeLabel[11];
		u8 FatTypeLabel[8];
	} __attribute__((packed));

	struct BootRecordExtension32 {
		u32 NumSectorsPerFAT;
		u16 ExtendFlags;
		u16 FatVersion;
		u32 RootCluster;
		u16 FATInformation;
		/// Location of backup of boot record (in case of bad read/corruption).
		u16 BackupBootRecordSector;
		u8 Reserved0[12];
		u8 DriveNumber;
		u8 Reserved1;
		u8 BootSignature;
		u32 VolumeID;
		u8 VolumeLabel[11];
		u8 FatTypeLabel[8];
	} __attribute__((packed));

	/// Boot Record
	///   Starting at logical sector zero of the partition, occupies one sector.
	///   Contains both data and code mixed together.
	struct BootRecord {
		// See above.
		BIOSParameterBlock BPB;
		// This will be cast to it's specific type once the driver parses
		//   what type of FAT this is (extended 16 or extended 32).
		u8 Extended[54];
	} __attribute__((packed));

	enum FATType {
		INVALID = 0,
		FAT12 = 1,
		FAT16 = 2,
		FAT32 = 3,
		ExFAT = 4
	};

	class FATDevice {
	public:
		AHCI::Port* Port;
		FATType Type {INVALID};
		BootRecord BR;

		FATDevice()  {}
		~FATDevice() {}

		inline u32 get_total_sectors() {
			if (BR.BPB.TotalSectors16 == 0) {
				return BR.BPB.TotalSectors32;
			}
			return BR.BPB.TotalSectors16;
		}

		inline u32 get_total_fat_sectors() {
			if (BR.BPB.NumSectorsPerFAT == 0) {
				return (*(BootRecordExtension32*)&BR.Extended).NumSectorsPerFAT;
			}
			return BR.BPB.NumSectorsPerFAT;
		}

		inline u32 get_first_fat_sector() {
			return BR.BPB.NumReservedSectors;
		}

		u32 get_root_directory_sectors() {
			static u32 sRootDirSectors = 0;
			if (sRootDirSectors == 0) {
			    sRootDirSectors = ((BR.BPB.NumEntriesInRoot * FAT_DIRECTORY_SIZE_BYTES)
								   + (BR.BPB.NumBytesPerSector-1)) / BR.BPB.NumBytesPerSector;
			}
			return sRootDirSectors;
		}

		inline u32 get_first_data_sector() {
			static u32 sFirstDataSector = 0;
			if (sFirstDataSector == 0) {
				sFirstDataSector = BR.BPB.NumReservedSectors
					+ (BR.BPB.NumFATsPresent * get_total_fat_sectors())
					+ get_root_directory_sectors();
			}
			return sFirstDataSector;
		}

		inline u32 get_root_directory_start_sector() {
			if (Type == FATType::FAT12 || Type == FATType::FAT16) {
				return get_first_data_sector() - get_root_directory_sectors();				
			}
			else {
				return (*(BootRecordExtension32*)&BR.Extended).RootCluster;
			}
		}

		u32 get_total_data_sectors() {
			static u32 sTotalDataSectors = 0;
			if (sTotalDataSectors == 0) {
			    sTotalDataSectors = get_total_sectors()
				- (BR.BPB.NumReservedSectors
				   + (BR.BPB.NumFATsPresent * get_total_fat_sectors())
				   + get_root_directory_sectors());
			}
			return sTotalDataSectors;
		}

		u32 get_total_clusters() {
			static u32 sTotalClusters = 0;
			if (sTotalClusters == 0) {
				// This rounds down.
			    sTotalClusters = get_total_data_sectors()
					/ BR.BPB.NumSectorsPerCluster;
			}
			return sTotalClusters;
		}

		inline u32 get_cluster_start_sector(u32 cluster) {
			return ((cluster - 2) * BR.BPB.NumSectorsPerCluster)
				+ get_first_data_sector();
		}

		/// Return total size of all sectors formatted in bytes.
		u64 get_total_size() {
			static u64 sTotalSize = 0;
			if (sTotalSize == 0) {
				sTotalSize = get_total_sectors() * BR.BPB.NumBytesPerSector;
			}
			return sTotalSize;
		}

		u64 get_data_size() {
			static u64 sDataSize = 0;
			if (sDataSize == 0) {
				sDataSize = get_total_data_sectors() * BR.BPB.NumBytesPerSector;
			}
			return sDataSize;
		}
	};

	void srl_fat_boot_record(FATDevice* device) {
		u64 totalSectors     = (u64)device->get_total_sectors();
		u64 totalDataSectors = (u64)device->get_total_data_sectors();
		srl.writestr("FAT Boot Record: \r\n");
		srl.writestr("|\\\r\n");
		srl.writestr("| Sector Size: ");
		srl.writestr(to_string((u64)device->BR.BPB.NumBytesPerSector));
		srl.writestr("\r\n");
		srl.writestr("| |\\\r\n");
		srl.writestr("| | Total sectors: ");
		srl.writestr(to_string(totalSectors));
		srl.writestr("\r\n");
		srl.writestr("| | \\\r\n");
		srl.writestr("| |  Total size: ");
		srl.writestr(to_string(device->get_total_size() / 1024 / 1024));
		srl.writestr("MiB\r\n");
		srl.writestr("| \\\r\n");
		srl.writestr("|  Total data sectors: ");
		srl.writestr(to_string(totalDataSectors));
		srl.writestr("\r\n");
		srl.writestr("|  \\\r\n");
		srl.writestr("|   Total data size: ");
		srl.writestr(to_string(device->get_data_size() / 1024 / 1024));
		srl.writestr("MiB\r\n");
		srl.writestr(" \\\r\n");
		srl.writestr("  Number of Sectors Per Cluster: ");
		srl.writestr(to_string((u64)device->BR.BPB.NumSectorsPerCluster));
		srl.writestr("\r\n");
	}

	/// The FAT Driver will house all functionality pertaining to actually
	///   reading and writing to/from a FATDevice.
	/// This includes:
	///   - Parsing a port to see if it is an eligible FAT device.
	///   - Reading/Writing a file.
	///   - Reading/Writing a directory.
	class FATDriver {
	public:
		// TODO: Move array to dynamically allocated memory (new + delete)
		FATDevice devices[32];
		u8 numDevices{0};

		/// Read the first logical sector of the device (boot sector),
		///   then validate that it matches what's expected of a FAT filesystem.
		/// If it doesn't match, set devices `Type` field to `INVALID` as a flag.
	    void read_boot_sector(u8 index) {
			srl.writestr("[FatFS]: Reading boot sector\r\n");
		    if (devices[index].Port->Read(0, 1, devices[index].Port->buffer)) {
				memcpy((void*)devices[index].Port->buffer, &devices[index].BR, 720);
				// FAT Filesystem magic bytes (0xaa55 word at 0x1fe offset).
				// Ensure there is at least one fat present
				if (*(u16*)((u64)devices[index].Port->buffer + 0x1fe) != 0xaa55
					|| devices[index].BR.BPB.NumFATsPresent == 0
					|| (devices[index].BR.BPB.NumBytesPerSector
						& (devices[index].BR.BPB.NumBytesPerSector - 1) == 0))
				{
					devices[index].Type = FATType::INVALID;
					return;
				}
				// TODO: More fool-proof method of detecting FAT type.
				// Get FAT type based on total cluster amount (mostly correct).
				u32 totalClusters = devices[index].get_total_clusters();
				if (totalClusters == 0) {
					devices[index].Type = FATType::ExFAT;
				}
				else if (totalClusters < 4085) {
					devices[index].Type = FATType::FAT12;
				}
				else if (totalClusters < 65525) {
					devices[index].Type = FATType::FAT16;
				}
				else {
					devices[index].Type = FATType::FAT32;
				}

				srl_fat_boot_record(&devices[index]);
			}
			else {
			    srl.writestr("[FatFS]: Unsuccessful read (is device functioning properly?)\r\n");
			}
		}

		bool is_device_fat(AHCI::Port* port) {
			u8 devIndex = numDevices;
			numDevices++;

			devices[devIndex].Port = port;
			
			// Read boot sector from devices' port.
			read_boot_sector(devIndex);
			if (devices[devIndex].Type == FATType::INVALID) {
				numDevices--;
				return false;
			}
			return true;
		}

		void read_root_directory(u8 index) {
			if (devices[index].Type == FATType::FAT12
				|| devices[index].Type == FATType::FAT16)
			{
				//TODO:READ ROOT DIRECTORY
				// FAT12/FAT16 have fixed root directory position.
				//   first_root_dir_sector = first_data_sector - root_dir_sectors;
			}
			else if (devices[index].Type == FATType::FAT32
					 || devices[index].Type == FATType::ExFAT)
			{
				//TODO:READ ROOT DIRECTORY
				// FAT32/ExFAT store root directory in a cluster, which can be
				//   found by accessing the boot record extension `RootCluster` field.
			}
		}
	};
}

#endif
