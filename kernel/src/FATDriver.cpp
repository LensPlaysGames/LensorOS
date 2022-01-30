#include "FATDriver.h"
#include "ahci.h"
#include "FAT_definitions.h"
#include "vfs_inode.h"

FATDriver gFATDriver;

void FATDriver::read_to_inode(AHCI::AHCIDriver* ahci, u8 portNumber, Inode* inode) {
	return;
}

void FATDriver::write_from_inode(AHCI::AHCIDriver* ahci, u8 portNumber, Inode* inode) {
	return;
}

u32 FATDriver::get_total_sectors(BootRecord* BR) {
	if (BR->BPB.TotalSectors16 == 0) {
		return BR->BPB.TotalSectors32;
	}
	return BR->BPB.TotalSectors16;
}

u32 FATDriver::get_total_fat_sectors(BootRecord* BR) {
	if (BR->BPB.NumSectorsPerFAT == 0) {
		return (*(BootRecordExtension32*)&BR->Extended).NumSectorsPerFAT;
	}
	return BR->BPB.NumSectorsPerFAT;
}

u32 FATDriver::get_root_directory_sectors(BootRecord* BR) {
	return ((BR->BPB.NumEntriesInRoot * FAT_DIRECTORY_SIZE_BYTES)
			+ (BR->BPB.NumBytesPerSector-1))
		/ BR->BPB.NumBytesPerSector;
}

u32 FATDriver::get_total_data_sectors(BootRecord* BR) {
	return get_total_sectors(BR)
		- (BR->BPB.NumReservedSectors
		   + (BR->BPB.NumFATsPresent * get_total_fat_sectors(BR))
		   + get_root_directory_sectors(BR));
}

u32 FATDriver::get_first_data_sector(BootRecord* BR) {
	return BR->BPB.NumReservedSectors
		+ (BR->BPB.NumFATsPresent * get_total_fat_sectors(BR))
		+ get_root_directory_sectors(BR);
}

u32 FATDriver::get_total_clusters(BootRecord* BR) {
	return get_total_data_sectors(BR) / BR->BPB.NumSectorsPerCluster;
}

u32 FATDriver::get_first_sector_in_cluster(BootRecord* BR, u32 cluster_number) {
	return ((cluster_number - 2) * BR->BPB.NumSectorsPerCluster) + get_first_data_sector(BR);
}

FATType FATDriver::get_type(BootRecord* BR) {
	// TODO: More fool-proof method of detecting FAT type.
	// Get FAT type based on total cluster amount (mostly correct).
	u32 totalClusters = get_total_clusters(BR);
	if (totalClusters == 0) {
		return FATType::ExFAT;
	}
	else if (totalClusters < 4085) {
		return FATType::FAT12;
	}
	else if (totalClusters < 65525) {
		return FATType::FAT16;
	}
	else {
		return FATType::FAT32;
	}
}

/// Try to parse a FAT boot record from the first boot sector of the SATA device.
/// This is used by the AHCI driver to determine which file system to create for a given device.
bool FATDriver::is_device_valid_filesystem(AHCI::AHCIDriver* ahci, u8 portNumber) {
	bool result = true;
	if (ahci->Ports[portNumber]->Read(0, 1, ahci->Ports[portNumber]->buffer)) {
		// Read successful.
		// Allocate memory for a FAT boot record.
		BootRecord* br = new BootRecord;
		// Copy data read from boot sector into FAT boot record.
		srl.writestr(to_string((u64)sizeof(BootRecord)));
		memcpy((void*)ahci->Ports[portNumber]->buffer, (void*)br, sizeof(BootRecord));
		// Validate boot sector is of FAT format.
		// Thanks to Gigasoft of osdev forums for this list
		// What makes a FAT filesystem valid ([x] means it's something this driver checks.):
		// [x] Word at 0x1fe (510) equates to 0xaa55
		// [ ] Sector size is power of two between 512-4096 (inclusive)
		// [ ] Cluster size is a power of two
		// [ ] Media type is 0xf0 or greater or equal to 0xf8
		// [ ] FAT size is not zero
		// [x] Number of sectors is not zero
		// [ ] Number of root directory entries is (zero if fat32) (not zero if fat12/16)
		// [ ] Root cluster is valid (FAT32)
		// [ ] File system version is zero (FAT32)
		// [x] NumFATsPresent greater than zero
		if (*(u16*)((u64)ahci->Ports[portNumber]->buffer + 510) != 0xaa55
			|| br->BPB.NumFATsPresent <= 0
			|| get_total_sectors(br) == 0)
		{
			result = false;
		}
		delete br;
	}
	else {
		// Read failed.
		result = false;
	}
	return result;
}
