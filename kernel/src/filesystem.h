#ifndef LENSOR_OS_FILESYSTEM_H
#define LENSOR_OS_FILESYSTEM_H

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
    FileSystemDriver* Driver;

	virtual void read(Inode* inode) {
		Driver->read_to_inode(AHCI, PortNumber, inode);
	}
	
	virtual void write(Inode* inode) {
		Driver->write_from_inode(AHCI, PortNumber, inode);
	}

	/// TOTAL SIZE IN BYTES
	virtual u64 get_total_size() { return 0; }
};

#endif
