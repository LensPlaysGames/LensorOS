#ifndef LENSOR_OS_FAT_DRIVER_H
#define LENSOR_OS_FAT_DRIVER_H

#include "integers.h"
#include "fat_definitions.h"

enum class FATType;

namespace AHCI {
    class AHCIDriver;
}
class Inode;

/// The FAT driver wraps around the AHCI driver and parse data into/out of VFS format (inode).
class FATDriver {
public:
    FATDriver() {}
    
    void read_to_inode           (AHCI::AHCIDriver*, u8 portNumber, Inode*);
    void write_from_inode        (AHCI::AHCIDriver*, u8 portNumber, Inode*);
    bool is_device_fat_formatted (AHCI::AHCIDriver*, u8 portNumber);
    void read_root_dir           (AHCI::AHCIDriver*, u8 portNumber, BootRecord*, FATType type);

    u32 get_total_sectors(BootRecord* BR) const {
        if (BR->BPB.TotalSectors16 == 0)
            return BR->BPB.TotalSectors32;
        return BR->BPB.TotalSectors16;
    }
    
    inline u32 get_total_fat_sectors           (BootRecord*)                    const;
    inline u32 get_root_directory_sectors      (BootRecord*)                    const;
    inline u32 get_first_root_directory_sector (BootRecord*, FATType)           const;
    inline u32 get_root_directory_cluster      (BootRecord*, FATType)           const;
    inline u32 get_total_data_sectors          (BootRecord*)                    const;
    inline u32 get_first_data_sector           (BootRecord*)                    const;
    inline u32 get_total_clusters              (BootRecord*)                    const;
    inline u32 get_first_sector_in_cluster     (BootRecord*, u32 clusterNumber) const;
    inline u32 get_cluster_from_first_sector   (BootRecord* BR, u64 sector)     const;

    FATType get_type (BootRecord* br) const;
};
extern FATDriver gFATDriver;

#endif
