#ifndef LENSOR_OS_FILESYSTEM_H
#define LENSOR_OS_FILESYSTEM_H

#include "pure_virtuals.h"

namespace AHCI {
	class AHCIDriver;
}
class Inode;

enum class FileSystemFormat {
	UNKNOWN = 0,
	INVALID = 1,
	FAT = 2
};

class FileSystem {
public:
	FileSystemFormat Format;
	AHCI::AHCIDriver* AHCI;
    u8 PortNumber;

	FileSystem() {}
	virtual ~FileSystem() {}

	virtual void read (Inode* inode) = 0;
	virtual void write(Inode* inode) = 0;

	/// TOTAL SIZE IN BYTES
	virtual u64 get_total_size() { return 0; }
};

#endif
