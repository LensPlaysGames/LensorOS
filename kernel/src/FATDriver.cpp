#include "FATDriver.h"
#include "ahci.h"
#include "FAT_definitions.h"
#include "vfs_inode.h"

FATDriver gFATDriver;

void FATDriver::read_to_inode(AHCI::AHCIDriver* ahci, u8 portNumber, Inode* inode) {
    // TODO: Implement reading a FAT file/directory (world) into an inode.
    (void)ahci;
    (void)portNumber;
    (void)inode;
    return;
}

void FATDriver::write_from_inode(AHCI::AHCIDriver* ahci, u8 portNumber, Inode* inode) {
    (void)ahci;
    (void)portNumber;
    (void)inode;
    return;
}

u32 FATDriver::get_total_fat_sectors(BootRecord* BR) const {
    if (BR->BPB.NumSectorsPerFAT == 0)
        return (*(BootRecordExtension32*)&BR->Extended).NumSectorsPerFAT;
    return BR->BPB.NumSectorsPerFAT;
}

u32 FATDriver::get_root_directory_sectors(BootRecord* BR) const {
    if (BR->BPB.NumSectorsPerFAT == 0)
        return 0;
    return ((BR->BPB.NumEntriesInRoot * FAT_DIRECTORY_SIZE_BYTES) + (BR->BPB.NumBytesPerSector-1)) / BR->BPB.NumBytesPerSector;
}

u32 FATDriver::get_first_root_directory_sector(BootRecord* BR, FATType type) const {
    if (type == FATType::FAT12 || type == FATType::FAT16)
        return get_first_data_sector(BR) + get_root_directory_sectors(BR);
    else if (type == FATType::FAT32 || type == FATType::ExFAT)
        return get_first_sector_in_cluster(BR, ((BootRecordExtension32*)BR->Extended)->RootCluster);
    // TODO: Handle invalid type (some sort of ErrorOr<T> template would be nice).
    return -1;
}

u32 FATDriver::get_root_directory_cluster(BootRecord* BR, FATType type) const {
    if (type == FATType::FAT12 || type == FATType::FAT16)
        return get_cluster_from_first_sector(BR, (get_first_data_sector(BR) - get_root_directory_sectors(BR)));
    else if (type == FATType::FAT32 || type == FATType::ExFAT)
        return ((BootRecordExtension32*)BR->Extended)->RootCluster;
    // TODO: Handle invalid type.
    return -1;
}

u32 FATDriver::get_total_data_sectors(BootRecord* BR) const {
    return get_total_sectors(BR)
        - (BR->BPB.NumReservedSectors
           + (BR->BPB.NumFATsPresent * get_total_fat_sectors(BR))
           + get_root_directory_sectors(BR));
}

u32 FATDriver::get_first_data_sector(BootRecord* BR) const {
    return BR->BPB.NumReservedSectors
        + (BR->BPB.NumFATsPresent * get_total_fat_sectors(BR))
        + get_root_directory_sectors(BR);
}

u32 FATDriver::get_total_clusters(BootRecord* BR) const {
    return get_total_data_sectors(BR) / BR->BPB.NumSectorsPerCluster;
}

u32 FATDriver::get_first_sector_in_cluster(BootRecord* BR, u32 cluster_number) const {
    return ((cluster_number - 2) * BR->BPB.NumSectorsPerCluster) + get_first_data_sector(BR);
}

u32 FATDriver::get_cluster_from_first_sector(BootRecord* BR, u64 sector) const {
    return ((sector - get_first_data_sector(BR)) / BR->BPB.NumSectorsPerCluster) + 2;
}

FATType FATDriver::get_type(BootRecord* BR) const {
    // TODO: More fool-proof method of detecting FAT type.
    // Get FAT type based on total cluster amount (mostly correct).
    u32 totalClusters = get_total_clusters(BR);
    if (totalClusters == 0)
        return FATType::ExFAT;
    else if (totalClusters < 4085)
        return FATType::FAT12;
    else if (totalClusters < 65525)
        return FATType::FAT16;
    return FATType::FAT32;
}

/// Try to parse a FAT boot record from the first boot sector of the SATA device.
/// This is used by the AHCI driver to determine which file system to create for a given device.
bool FATDriver::is_device_fat_formatted(AHCI::AHCIDriver* ahci, u8 portNumber) {
    bool result = true;
    if (ahci->Ports[portNumber]->Read(0, 1, ahci->Ports[portNumber]->buffer)) {
        // Read successful.
        // Allocate memory for a FAT boot record.
        BootRecord* br = new BootRecord;
        // Copy data read from boot sector into FAT boot record.
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
        bool invalid = (*(u16*)((u64)ahci->Ports[portNumber]->buffer + 510) != 0xaa55
                        || br->BPB.NumFATsPresent <= 0 || get_total_sectors(br) == 0);
        if (invalid)
            result = false;
        delete br;
    }
    else result = false;
    return result;
}

void FATDriver::read_root_dir(AHCI::AHCIDriver* ahci, u8 portNumber, BootRecord* BR, FATType type) {
    u32 clusterIndex = get_root_directory_cluster(BR, type);
    bool lfnBufferFull {false};
    u16* lfnBuffer = new u16[13];
    u8* FAT = new u8[BR->BPB.NumBytesPerSector];
    ClusterEntry* clEntry;
    u32 clusterNumber {0};
    u32 FAToffset     {0};
    u32 FATsector     {0};
    u32 lastFATsector {0};
    u32 entryOffset   {0};
    u32 tableValue    {0};
    while (true) {
        srl->writestr("[FATDriver]: \r\n  Reading cluster ");
        srl->writestr(to_string(clusterIndex));
        srl->writestr("\r\n");
        ahci->Ports[portNumber]->Read(get_first_sector_in_cluster(BR, clusterIndex),
                                      BR->BPB.NumSectorsPerCluster,
                                      ahci->Ports[portNumber]->buffer);
        clEntry = (ClusterEntry*)ahci->Ports[portNumber]->buffer;
        lfnBufferFull = false;
        while (true) {
            if (clEntry->FileName[0] == 0)
                break;
            if (clEntry->FileName[0] == 0xe5)
                continue;

            // Detect a long file name entry
            if (clEntry->Attributes == 0x0f) {
                LFNClusterEntry* lfn = (LFNClusterEntry*)clEntry;
                // Copy long file name into buffer.
                // First five two-byte characters.
                memcpy(&lfn->Characters1[0], &lfnBuffer[0], 10);
                // Next six characters.
                memcpy(&lfn->Characters2[0], &lfnBuffer[5], 12);
                // Last two characters.
                memcpy(&lfn->Characters3[0], &lfnBuffer[11], 4);
                lfnBufferFull = true;
                clEntry++;
                continue;
            }

            bool is_file = false;
            /// Read Only: 0b00000001
            /// Hidden:    0b00000010
            /// System:    0b00000100
            /// Volume ID: 0b00001000
            /// Directory: 0b00010000
            /// Archive:   0b00100000
            srl->writestr("   ");
            if (clEntry->Attributes & 0b00000001)
                srl->writestr(" Read-only ");
            if (clEntry->Attributes & 0b00000010)
                srl->writestr(" Hidden ");
            if (clEntry->Attributes & 0b00000100)
                srl->writestr(" System ");
            else srl->writeb((u8)' ');
            
            if (clEntry->Attributes & 0b00010000)
                srl->writestr("Directory");
            else if (clEntry->Attributes & 0b00001000)
                srl->writestr("Volume Label");
            else {
                is_file = true;
                srl->writestr("File");
            }

            // Write file name
            if (lfnBufferFull) {
                // TODO: Apply long file name to current entry...
                //       This should be stored in VFS structure for later use.
                srl->writestr(" with long name: ");
                srl->writestr((char*)&lfnBuffer[0], 26);
                srl->writestr("\r\n");
                lfnBufferFull = false;
            }
            else {
                srl->writestr(": ");
                srl->writestr((char*)&clEntry->FileName[0], 11);
                srl->writestr("\r\n");
            }

            if (is_file) {
                srl->writestr("      File Size: ");
                srl->writestr(to_string(clEntry->FileSizeInBytes / 1024 / 1024));
                srl->writestr(" MiB (");
                srl->writestr(to_string(clEntry->FileSizeInBytes / 1024));
                srl->writestr(" KiB)\r\n");
            }
            
            // Increment to next entry in cluster.
            clEntry++;
        }

        // Check if this is last cluster in chain.
        clusterNumber = clEntry->ClusterNumberL;
        clusterNumber |= (u32)clEntry->ClusterNumberH << 16;

        if (type == FATType::FAT12)
            FAToffset = clusterNumber + (clusterNumber / 2);
        else if (type == FATType::FAT16)
            FAToffset = clusterNumber * 2;
        else if (type == FATType::FAT32 || type == FATType::ExFAT)
            FAToffset = clusterNumber * 4;

        FATsector = BR->BPB.NumReservedSectors + (FAToffset / BR->BPB.NumBytesPerSector);
        entryOffset = FAToffset % BR->BPB.NumBytesPerSector;
        
        // Entries under index 0 and 1 are reserved.
        // Don't read same file twice in a row.
        if (entryOffset <= 1 || FATsector == lastFATsector)
            break;

        ahci->Ports[portNumber]->Read(FATsector, 1, (void*)FAT);
        lastFATsector = FATsector;

        if (type == FATType::FAT12) {
            tableValue = *(u32*)((u16*)&FAT[entryOffset]);
            /* If table value is greater or equal to 0x0ff8,
             *   then there are no more clusters in the cluster chain (entire file read).
             * Else if table value is equal to 0x0ff7, then this cluster is marked 
             *   as "bad", meaning it is prone to errors and should be avoided.
             * Else, table value contains the cluster number of the next cluster in the file. 
             */
            if (clusterNumber & 0b1)
                tableValue = tableValue >> 4;
            else tableValue = tableValue & 0x0fff;
            
            if (tableValue == 0x0ff8)
                break;
            else if (tableValue == 0x0ff7)
                continue;
        }
        else if (type == FATType::FAT16) {
            tableValue = *(u32*)((u16*)&FAT[entryOffset]);
            /* If table value is greater or equal to 0xfff8,
             *   then there are no more clusters in the cluster chain (entire file read).
             * Else if table value is equal to 0xfff7, then this cluster is marked 
             *   as "bad", meaning it is prone to errors and should be avoided.
             * Else, table value contains the cluster number of the next cluster in the file. 
             */
            if (tableValue == 0xfff8)
                break;
            else if (tableValue == 0xfff7)
                continue;
        }
        else if (type == FATType::FAT32) {
            tableValue = *(u32*)&FAT[entryOffset];
            tableValue &= 0x0fffffff;
            /* If table value is greater or equal to 0x0ffffff8,
             *   then there are no more clusters in the cluster chain (entire file read).
             * Else if table value is equal to 0x0ffffff7, then this cluster is marked 
             *   as "bad", meaning it is prone to errors and should be avoided.
             * Else, table value contains the cluster number of the next cluster in the file. 
             */
            if (tableValue >= 0x0ffffff8)
                break;
            else if (tableValue == 0x0ffffff7)
                continue;
        }
        else if (type == FATType::ExFAT) {
            tableValue = *(u32*)&FAT[entryOffset];
            /* If table value is greater or equal to 0xfffffff8,
             *   then there are no more clusters in the cluster chain (entire file read).
             * Else if table value is equal to 0xfffffff7, then this cluster is marked 
             *   as "bad", meaning it is prone to errors and should be avoided.
             * Else, table value contains the cluster number of the next cluster in the file. 
             */
            if      (tableValue >= 0xfffffff8) { break;    }
            else if (tableValue == 0xfffffff7) { continue; }
        }
        
        // Read next cluster based on tableValue giving cluster number
        clusterIndex = tableValue;
    }
    delete[] lfnBuffer;
    delete[] FAT;
}
