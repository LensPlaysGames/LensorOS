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
	FATDriver* Driver;
	BootRecord BR;
	FATType Type;
	
	FatFS(AHCI::AHCIDriver* ahci, u8 portNumber) {
		Format = FileSystemFormat::FAT;
		AHCI = ahci;
		PortNumber = portNumber;
		Driver = &gFATDriver;
		// Get boot record from device.
		if (ahci->Ports[portNumber]->Read(0, 1, ahci->Ports[portNumber]->buffer)) {
			memcpy((void*)ahci->Ports[portNumber]->buffer, &BR, sizeof(BootRecord));
			// Set type based on boot record information.
			Type = ((FATDriver*)Driver)->get_type(&BR);
		}
		else {
			// Read from device failed.
			srl.writestr("[FatFS]: ERROR -> Could not read from device at port ");
			srl.writestr(to_string((u64)portNumber));
			srl.writestr("\r\n");
		}
	}

	virtual ~FatFS() {}

	void read (Inode* inode) override {
		// TODO: Read from FAT file-system based on VFS intermediate-representation's data.
		(void)inode;

		// For now this just lists the files in the root directory.
		Driver->read_root_dir(AHCI, PortNumber, &BR, Type);
	}

	void write(Inode* inode) override {
		// TODO: Write to FAT file-system based on VFS intermediate-representation's data.
		(void)inode;
	}

	u64 get_total_size() override {
		return Driver->get_total_sectors(&BR) * BR.BPB.NumBytesPerSector;
	}
};

#endif
