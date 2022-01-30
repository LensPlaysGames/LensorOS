#ifndef LENSOR_OS_FAT_FS_H
#define LENSOR_OS_FAT_FS_H

#include "filesystem.h"
#include "FAT_definitions.h"

/// The FAT File System
/// FAT = File Allocation Table

// Resource Used: https://wiki.osdev.org/FAT

/// This class will be created for each FAT-formatted file-system found using the AHCI driver.
/// It will store information about the device (size,)
class FatFS : public FileSystem {
public:
	BootRecord BR;
	FATType Type;
	
	FatFS(AHCI::AHCIDriver* ahci, u8 portNumber) {
		Format = FileSystemFormat::FAT;
		AHCI = ahci;
		PortNumber = portNumber;
		Driver = (FileSystemDriver*)&gFATDriver;
		// Get boot record from device.
		if (ahci->Ports[portNumber]->Read(0, 1, ahci->Ports[portNumber]->buffer)) {
			memcpy((void*)ahci->Ports[portNumber]->buffer, &BR, sizeof(BootRecord));
			// Set type based on boot record information.
			Type = ((FATDriver*)Driver)->get_type(&BR);
		}
		else {
			// Read from device failed.
		}
	}

	u64 get_total_size() override {
		return ((FATDriver*)Driver)->get_total_sectors(&BR) * BR.BPB.NumBytesPerSector;
	}
};

#endif
