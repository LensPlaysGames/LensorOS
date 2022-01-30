#ifndef LENSOR_OS_FILESYSTEM_DRIVER_H
#define LENSOR_OS_FILESYSTEM_DRIVER_H

#include "integers.h"

namespace AHCI {
	class AHCIDriver;
}

class Inode;

class FileSystemDriver {
public:
	virtual void initialize() {}
	virtual void destroy()    {}
	virtual void read_to_inode (AHCI::AHCIDriver* ahci, u8 portNumber, Inode* inode)    {}
	virtual void write_from_inode (AHCI::AHCIDriver* ahci, u8 portNumber, Inode* inode) {}
	virtual bool is_device_valid_filesystem(AHCI::AHCIDriver* achi, u8 portNumber) { return false; }
};
#endif
