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
		ExtendedBootRecord16 ExtBR16;
		ExtendedBootRecord32 ExtBR32;

		inline uint32_t get_total_sectors() {
			if (BPB.TotalSectors16 == 0) {
				return BPB.TotalSectors32;
			}
			return BPB.TotalSectors16;
		}

		inline uint32_t get_fat_size_sectors() {
			if (BPB.NumSectorsPerFAT == 0) {
				return ExtBR32.NumSectorsPerFAT;
			}
			return BPB.NumSectorsPerFAT;
		}

		inline uint32_t get_first_FAT_sector() {
			return BPB.NumReservedSectors;
		}

		inline uint32_t get_root_directory_sectors() {
			return ((BPB.NumEntriesInRoot * FAT_DIRECTORY_SIZE_BYTES)
					+ (BPB.NumBytesPerSector - 1)) / BPB.NumBytesPerSector;
		}

		inline uint32_t get_first_data_sector() {
			return BPB.NumReservedSectors
				+ (BPB.NumFATsPresent * get_fat_size_sectors())
				+ get_root_directory_sectors();
		}

		inline uint32_t get_root_directory_start_sector() {
			return get_first_data_sector() - get_root_directory_sectors();
		}

		inline uint32_t get_total_data_sectors() {
			return get_total_sectors()
				- (BPB.NumReservedSectors
				   + (BPB.NumFATsPresent * get_fat_size_sectors())
				   + get_root_directory_sectors());
		}

		inline uint32_t get_total_clusters() {
			// This rounds down.
			return get_total_data_sectors() / BPB.NumSectorsPerCluster;
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
		gRend.putstr(to_string((uint64_t)(totalSectors
										  * device->BPB.NumBytesPerSector)
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
		gRend.putstr(to_string((uint64_t)(totalDataSectors
										  * device->BPB.NumBytesPerSector)
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

	    void read_boot_sector(uint8_t index, AHCI::Port* port) {
			devices[index].Port = port;
			// read boot sector from port into device at index.
			gRend.putstr("Reading boot sector");
			gRend.crlf();
		    if (port->Read(0, 1, port->buffer)) {
				memcpy(port->buffer, &devices[index].BPB, 512);
				// TODO: Validate that media is FAT formatted (how do I do this?)
				//         I guess I just have to ensure that values make sense...
				print_fat_boot_record(&devices[index]);
			}
			else {
				gRend.putstr("Unsuccessful read");
				gRend.crlf();
			}
		}

		bool is_device_FAT(AHCI::Port* port) {
			uint8_t devIndex = numDevices;
			numDevices++;
			// Read boot sector from port into device
			read_boot_sector(devIndex, port);

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
