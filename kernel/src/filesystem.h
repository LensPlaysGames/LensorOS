#ifndef LENSOR_OS_FILESYSTEM_H
#define LENSOR_OS_FILESYSTEM_H

#include "integers.h"
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
    FileSystemFormat Format {FileSystemFormat::UNKNOWN};
    AHCI::AHCIDriver* AHCI;
    u8 PortNumber {0};

    FileSystem() {}
    virtual ~FileSystem() {}

    virtual void read (Inode* inode) = 0;
    virtual void write(Inode* inode) = 0;

    /// TOTAL SIZE IN BYTES
    virtual u64 get_total_size() { return 0; }
};

extern FileSystem** FileSystems;
extern u16 NumFileSystems;

#endif
