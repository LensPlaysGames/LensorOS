#ifndef LENSOR_OS_FAT_DRIVER_H
#define LENSOR_OS_FAT_DRIVER_H

#include "integers.h"
#include "filesystem_driver.h"
#include "FAT_definitions.h"

namespace AHCI {
	class AHCIDriver;
}

/// The FAT driver wraps around the AHCI driver and parse data into/out of VFS format (inode).
class FATDriver : public FileSystemDriver {
public:
	FATDriver() {}
	
	void read_to_inode (AHCI::AHCIDriver* ahci, u8 portNumber, Inode* inode)    override;
	void write_from_inode (AHCI::AHCIDriver* ahci, u8 portNumber, Inode* inode) override;
	bool is_device_valid_filesystem (AHCI::AHCIDriver* achi, u8 portNumber)     override;

	inline u32 get_total_sectors           (BootRecord* BR);
	inline u32 get_total_fat_sectors       (BootRecord* BR);
	inline u32 get_root_directory_sectors  (BootRecord* BR);
	inline u32 get_total_data_sectors      (BootRecord* BR);
	inline u32 get_first_data_sector       (BootRecord* BR);
	inline u32 get_total_clusters          (BootRecord* BR);
	inline u32 get_first_sector_in_cluster (BootRecord* BR, u32 cluster_number);
	
	FATType get_type (BootRecord* br);
};
extern FATDriver gFATDriver;

#endif
