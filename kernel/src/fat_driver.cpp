#include "fat_driver.h"

#include "ahci.h"
#include "cstr.h"
#include "fat_definitions.h"
#include "heap.h"
#include "inode.h"
#include "memory.h"
#include "smart_pointer.h"
#include "uart.h"

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
    if (ahci->Ports[portNumber]->Read(0, 1)) {
        // Read successful.
        // Allocate memory for a FAT boot record.
        SmartPtr<BootRecord> br = SmartPtr(new BootRecord);
        // Copy data read from boot sector into FAT boot record.
        memcpy((void*)ahci->Ports[portNumber]->buffer, (void*)br.get(), sizeof(BootRecord));
        /* Validate boot sector is of FAT format.
         * Thanks to Gigasoft of osdev forums for this list
         * TODO: Use more of these sanity checks before assuming it is valid FAT filesystem.
         * What makes a FAT filesystem valid ([x] means it's something this driver checks.):
         * [x] Word at byte offset 510 equates to 0xaa55
         * [ ] Sector size is power of two between 512-4096 (inclusive)
         * [ ] Cluster size is a power of two
         * [ ] Media type is 0xf0 or greater or equal to 0xf8
         * [ ] FAT size is not zero
         * [x] Number of sectors is not zero
         * [ ] Number of root directory entries is (zero if fat32) (not zero if fat12/16)
         * [ ] Root cluster is valid (FAT32)
         * [ ] File system version is zero (FAT32)
         * [x] NumFATsPresent greater than zero
         */
        bool invalid = (br->Magic != 0xaa55
                        || br->BPB.NumFATsPresent <= 0
                        || get_total_sectors(br.get()) == 0);
        if (invalid)
            result = false;
    }
    else result = false;
    return result;
}

// FIXME: This is spaghetti code, I need to re-do this.
//          FAT doesn't make it easy, but I can do it.
void FATDriver::read_directory
(
 AHCI::AHCIDriver* ahci
 , u8 portNumber
 , BootRecord* BR
 , FATType type
 , u32 directoryClusterIndex
 , u32 indentLevel)
{
    static const char* indent = "  ";
    u32 clusterIndex = directoryClusterIndex;
    bool lfnBufferFull { false };
    SmartPtr<u16[]> lfnBuffer(new u16[13], 13);
    SmartPtr<u8[]> FAT(new u8[BR->BPB.NumBytesPerSector], BR->BPB.NumBytesPerSector);
    ClusterEntry* current { nullptr };
    u32 clusterNumber { 0 };
    u32 FAToffset     { 0 };
    u32 FATsector     { 0 };
    u32 lastFATsector { 0 };
    u32 entryOffset   { 0 };
    u32 tableValue    { 0 };

    // Follow clusters while there are more in the chain.
    bool moreClusters { true };
    do {
        // Read cluster into AHCI buffer.
        u64 clusterSizeInBytes = BR->BPB.NumSectorsPerCluster * BR->BPB.NumBytesPerSector;
        SmartPtr<u8[]> clusterContents(new u8[clusterSizeInBytes], clusterSizeInBytes);
        if (ahci->Ports[portNumber]->Read(get_first_sector_in_cluster(BR, clusterIndex)
                                          , BR->BPB.NumSectorsPerCluster) == false)
        {
            srl->writestr(indent);
            for (u32 i = 0; i < indentLevel; ++i)
                srl->writestr(indent);
            srl->writestr("\033[31mCluster read failed.\033[0m\r\n");
            return;
        }
        memcpy((void*)ahci->Ports[portNumber]->buffer, (void*)clusterContents.get(), clusterSizeInBytes);
        current = (ClusterEntry*)clusterContents.get();
        // Get all entries within cluster.
        // The end is signified by an entry with
        //   the first byte of the file name set to zero.
        // Entries with an initial byte of 0xe5 are skipped.
        do {
            if (current->FileName[0] == 0xe5)
                continue;

            // Detect a long file name entry
            if (current->Attributes == 0x0f) {
                LFNClusterEntry* lfn = (LFNClusterEntry*)current;
                // Copy long file name into buffer.
                // First five two-byte characters.
                memcpy(&lfn->Characters1[0], &lfnBuffer[0], 10);
                // Next six characters.
                memcpy(&lfn->Characters2[0], &lfnBuffer[5], 12);
                // Last two characters.
                memcpy(&lfn->Characters3[0], &lfnBuffer[11], 4);
                lfnBufferFull = true;
                current++;
                continue;
            }

            // Parse attributes of file.
            bool is_dir = current->Attributes & FAT_ATTR_DIRECTORY;
            bool is_file = false;
            srl->writestr(indent);
            for (u32 i = 0; i < indentLevel; ++i)
                srl->writestr(indent);
            if (current->Attributes & FAT_ATTR_READ_ONLY)
                srl->writestr("Read-only ");
            if (current->Attributes & FAT_ATTR_HIDDEN)
                srl->writestr("Hidden ");
            if (current->Attributes & FAT_ATTR_SYSTEM)
                srl->writestr("System ");

            if (is_dir)
                srl->writestr("Directory");             
            else if (current->Attributes & FAT_ATTR_VOLUME_ID)
                srl->writestr("Volume Label");
            else if (current->Attributes & FAT_ATTR_ARCHIVE) {
                is_file = true;
                srl->writestr("Archive");
            }
            else {
                is_file = true;
                srl->writestr("File");
            }
            
            // Write file name to serial output.
            if (lfnBufferFull) {
                // Apply long file name to current entry.
                srl->writestr(" with long name: ");
                srl->writestr((char*)&lfnBuffer[0], 26);
                lfnBufferFull = false;
            }
            else {
                srl->writestr(": ");
                srl->writestr((char*)&current->FileName[0], 11);
            }
            srl->writestr("\r\n");

            if (is_file) {
                // Print file size.
                srl->writestr(indent);
                srl->writestr(indent);
                srl->writestr(indent);
                for (u32 i = 0; i < indentLevel; ++i)
                    srl->writestr(indent);
                srl->writestr("File Size: ");
                srl->writestr(to_string(current->FileSizeInBytes / 1024 / 1024));
                srl->writestr(" MiB (");
                srl->writestr(to_string(current->FileSizeInBytes / 1024));
                srl->writestr(" KiB)\r\n");
                // Print first 8 bytes of file.
                srl->writestr(indent);
                srl->writestr(indent);
                srl->writestr(indent);
                for (u32 i = 0; i < indentLevel; ++i)
                    srl->writestr(indent);
                srl->writestr("First 8 Bytes: \033[30;47m");
                SmartPtr<u8[]> buffer(new u8[8], 8);
                if (ahci->Ports[portNumber]->Read(get_first_sector_in_cluster(BR, current->get_cluster_number())
                                                  , BR->BPB.NumSectorsPerCluster))
                {
                    memcpy(ahci->Ports[portNumber]->buffer, buffer.get(), 8);
                    srl->writestr((char*)&buffer[0], 8);
                    srl->writestr("\033[0m\r\n");
                }
                else srl->writestr("\033[31mRead failed.\033[0m\r\n");
            }
            else if (is_dir && (strcmp((char*)&current->FileName[0], ".          ", 11)
                                || strcmp((char*)&current->FileName[0], "..         ", 11)) == false)
            {
                // If entry is directory and entry is not a subdirectory helper
                //   (ie: ".          " or "..         "), also read that directory recursively.
                // FIXME: Must be a better way of doing this than strcmp the name...
                read_directory(ahci, portNumber, BR, type, current->get_cluster_number(), indentLevel + 1);
            }

            current++;
        } while (current->FileName[0] != 0);

        // Check if this is last cluster in chain.
        clusterNumber = current->get_cluster_number();
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
        // Read File Allocation Table from disk (one sector).
        u16 sectorsPerFAT = BR->BPB.NumSectorsPerFAT == 0 ? 1 : BR->BPB.NumSectorsPerFAT;
        ahci->Ports[portNumber]->Read(FATsector, sectorsPerFAT);
        memcpy(ahci->Ports[portNumber]->buffer, FAT.get(), sectorsPerFAT * BR->BPB.NumBytesPerSector);
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
                moreClusters = false;
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
                moreClusters = false;
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
                moreClusters = false;
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
            if (tableValue >= 0xfffffff8)
                moreClusters = false;
            else if (tableValue == 0xfffffff7)
                continue;
        }
        // Read next cluster based on tableValue giving cluster number
        clusterIndex = tableValue;
    } while (moreClusters);
}

void FATDriver::read_root_directory(AHCI::AHCIDriver* ahci, u8 portNumber, BootRecord* BR, FATType type) {
    srl->writestr("[FATDriver]:\r\n");
    u32 clusterIndex = get_root_directory_cluster(BR, type);
    read_directory(ahci, portNumber, BR, type, clusterIndex);
}
