#ifndef LENSOR_OS_FILE_SYSTEM_FAT_H
#define LENSOR_OS_FILE_SYSTEM_FAT_H

#include "../ahci.h"
#include "../basic_renderer.h"

#define FAT_DIRECTORY_SIZE_BYTES 32

/// File Allocation Table File System
///   Formats storage media into three sections:
///     - Boot Record
///     - File Allocation Table (namesake)
///     - Directory + Data area (they couldn't name
///         this something cool like the other two?)
namespace FatFS {
	/// Boot Record
	///   Starting at logical sector zero of the partition, occupies one sector.
	///   Contains both data and code mixed together.
	/// BIOSParameterBlock
	///   Initial section of first logical sector on storage media.
	///   Contains information such as number of bytes per sector,
	///     num sectors per cluster, num reserved sectors, etc.
	struct BIOSParameterBlock {
		/// Infinite loop to catch a computer trying to
		///   boot from non-bootable drive: `EB FE 90`.
		uint8_t JumpCode[3];
		/// OEM Identifier
		uint8_t OEMID[8];
		uint16_t NumBytesPerSector;
		uint8_t NumSectorsPerCluster;
		/// Boot record sectors included in this value.
		uint16_t NumReservedSectors;
		uint8_t NumFATsPresent;
		uint16_t NumEntriesInRoot;
		/// Total sectors in logical volume.
		/// If zero, count is stored in `TotalSectors32`.
		uint16_t TotalSectors16;
		uint8_t MediaDescriptorType;
		/// FAT12/FAT16 ONLY.
		uint16_t NumSectorsPerFAT;
		uint16_t NumSectorsPerTrack;
		/// Number of heads or sides on the storage media.
		/// NOTE: Whatever program formatted the media may have been incorrect
		///         concerning the physical geometry of the media.
		uint16_t NumHeadsOrSides;
		/// Number of hidden sectors (the LBA of the beginning of the partition).
		uint32_t NumHiddenSectors;
		uint32_t TotalSectors32;
		// This will be cast to it's specific type once the driver parses
		//   what type of FAT this is (extended 16 or extended 32).
		uint8_t Extended[54];
	} __attribute__((packed));

	struct ExtendedBootRecord16 {
		uint8_t BIOSDriveNumber;
		uint8_t Reserved;
		uint8_t BootSignature;
		uint32_t VolumeID;
		uint8_t VolumeLabel[11];
		uint8_t FatTypeLabel[8];
	} __attribute__((packed));

	struct ExtendedBootRecord32 {
		uint32_t NumSectorsPerFAT;
		uint16_t ExtendFlags;
		uint16_t FatVersion;
		uint32_t RootCluster;
		uint16_t FATInformation;
		/// Location of backup of boot record (in case of bad read/corruption).
		uint16_t BackupBootRecordSector;
		uint8_t Reserved0[12];
		uint8_t DriveNumber;
		uint8_t Reserved1;
		uint8_t BootSignature;
		uint32_t VolumeID;
		uint8_t VolumeLabel[11];
		uint8_t FatTypeLabel[8];
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
		BIOSParameterBlock BPB;

		FATDevice()  {}
		~FATDevice() {}

		inline uint32_t get_total_sectors() {
			if (BPB.TotalSectors16 == 0) {
				return BPB.TotalSectors32;
			}
			return BPB.TotalSectors16;
		}

		inline uint32_t get_total_fat_sectors() {
			if (BPB.NumSectorsPerFAT == 0) {
				return (*(ExtendedBootRecord32*)&BPB.Extended).NumSectorsPerFAT;
			}
			return BPB.NumSectorsPerFAT;
		}

		inline uint32_t get_first_fat_sector() {
			return BPB.NumReservedSectors;
		}

		uint32_t get_root_directory_sectors() {
			static uint32_t sRootDirSectors = 0;
			if (sRootDirSectors == 0) {
			    sRootDirSectors = ((BPB.NumEntriesInRoot * FAT_DIRECTORY_SIZE_BYTES)
								   + (BPB.NumBytesPerSector-1)) / BPB.NumBytesPerSector;
			}
			return sRootDirSectors;
		}

		inline uint32_t get_first_data_sector() {
			static uint32_t sFirstDataSector = 0;
			if (sFirstDataSector == 0) {
				sFirstDataSector = BPB.NumReservedSectors
					+ (BPB.NumFATsPresent * get_total_fat_sectors())
					+ get_root_directory_sectors();
			}
			return sFirstDataSector;
		}

		inline uint32_t get_root_directory_start_sector() {
			return get_first_data_sector() - get_root_directory_sectors();
		}

		uint32_t get_total_data_sectors() {
			static uint32_t sTotalDataSectors = 0;
			if (sTotalDataSectors == 0) {
			    sTotalDataSectors = get_total_sectors()
				- (BPB.NumReservedSectors
				   + (BPB.NumFATsPresent * get_total_fat_sectors())
				   + get_root_directory_sectors());
			}
			return sTotalDataSectors;
		}

		uint32_t get_total_clusters() {
			static uint32_t sTotalClusters = 0;
			if (sTotalClusters == 0) {
				// This rounds down.
			    sTotalClusters = get_total_data_sectors() / BPB.NumSectorsPerCluster;
			}
			return sTotalClusters;
		}

		inline uint32_t get_cluster_start_sector(uint32_t cluster) {
			return ((cluster - 2) * BPB.NumSectorsPerCluster) + get_first_data_sector();
		}
	};

	void print_fat_boot_record(FATDevice* device) {
		uint64_t totalSectors = (uint64_t)device->get_total_sectors();
		uint64_t totalDataSectors = (uint64_t)device->get_total_data_sectors();
		gRend.putstr("FAT Boot Record: ");
		gRend.crlf();
		gRend.putstr("|\\");
		gRend.crlf();
		gRend.putstr("| Total Size: ");
		gRend.putstr(to_string(totalSectors * device->BPB.NumBytesPerSector
							   / 1024 / 1024));
		gRend.putstr("mib");
		gRend.crlf();
		gRend.putstr("| |\\");
		gRend.crlf();
		gRend.putstr("| | Total sectors: ");
		gRend.putstr(to_string(totalSectors));
		gRend.crlf();
		gRend.putstr("| \\");
		gRend.crlf();
		gRend.putstr("|  Sector Size: ");
		gRend.putstr(to_string((uint64_t)device->BPB.NumBytesPerSector));
		gRend.crlf();
		gRend.putstr("|\\");
		gRend.crlf();
		gRend.putstr("| Number of Sectors Per Cluster: ");
		gRend.putstr(to_string((uint64_t)device->BPB.NumSectorsPerCluster));
		gRend.crlf();
		gRend.putstr("|\\");
		gRend.crlf();
		gRend.putstr("| Total Usable Size: ");
		gRend.putstr(to_string(totalDataSectors * device->BPB.NumBytesPerSector
							   / 1024 / 1024));
		gRend.putstr("mib");
		gRend.crlf();
		gRend.putstr("| \\");
		gRend.crlf();
		gRend.putstr("|  Total data sectors: ");
		gRend.putstr(to_string(totalDataSectors));
		gRend.crlf();
	}

	/// The FAT Driver will house all functionality pertaining to actually
	///   reading and writing to/from a FATDevice.
	/// This includes:
	///   - Parsing a port to see if it is an eligible FAT device.
	///   - Reading/Writing a file.
	///   - Reading/Writing a directory.
	class FATDriver {
	public:
		FATDevice devices[32];
		uint8_t numDevices{0};

	    void read_boot_sector(uint8_t index) {
			// read boot sector from port into device at index.
			gRend.putstr("Reading boot sector");
			gRend.crlf();
		    if (devices[index].Port->Read(0, 1, devices[index].Port->buffer)) {
				memcpy((void*)devices[index].Port->buffer, &devices[index].BPB, 720);
				print_fat_boot_record(&devices[index]);
				if (devices[index].BPB.NumFATsPresent > 0) {
					uint32_t totalClusters = devices[index].get_total_clusters();
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
				}
			}
			else {
				gRend.putstr("Unsuccessful read");
				gRend.crlf();
			}
		}

		bool is_device_fat(AHCI::Port* port) {
			uint8_t devIndex = numDevices;
			numDevices++;

			devices[devIndex].Port = port;
			
			// Read boot sector from port into device.
			read_boot_sector(devIndex);
			if (devices[devIndex].Type == FATType::INVALID) {
				numDevices--;
				return false;
			}
			return true;
		}

		void read_root_directory() {
			
		}

		
	};
}

#endif
