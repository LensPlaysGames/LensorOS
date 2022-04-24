#include <storage/filesystem_drivers/file_allocation_table.h>

#include <integers.h>
#include <linked_list.h>
#include <smart_pointer.h>

bool FileAllocationTableDriver::test (StorageDeviceDriver* driver) {
    if (driver == nullptr)
        return false;

    auto buffer = SmartPtr<u8[]>(new u8[512], 512);
    if (buffer.get() == nullptr)
        return false;

    bool out { false };
    driver->read(0, 512, buffer.get());
    auto* br = reinterpret_cast<BootRecord*>(buffer.get());
    u64 totalSectors = br->BPB.TotalSectors16 == 0
        ? br->BPB.TotalSectors32 : br->BPB.TotalSectors16;
    /* Validate boot sector is of FAT format.
     * TODO: Use more of these confidence checks before
     *       assuming it is valid FAT filesystem.
     * What makes a FAT filesystem valid?
     * Thanks to Gigasoft of osdev forums for this list.
     * [x] = something this driver checks.
     * [x] Word at byte offset 510 equates to 0xaa55.
     * [x] Sector size is power of two between 512-4096 (inclusive).
     * [x] Cluster size is a power of two.
     * [ ] Media type is 0xf0 or greater or equal to 0xf8.
     * [ ] FAT size is not zero.
     * [x] Number of sectors is not zero.
     * [ ] Number of root directory entries is:
     *       - zero if FAT32.
     *       - not zero if FAT12 or FAT16.
     * [ ] (FAT32) Root cluster is valid
     * [ ] (FAT32) File system version is zero
     * [x] NumFATsPresent greater than zero
     */
    out = (br->Magic == 0xaa55
           && totalSectors != 0
           && br->BPB.NumBytesPerSector >= 512
           && br->BPB.NumBytesPerSector <= 4096
           && (br->BPB.NumBytesPerSector
               & (br->BPB.NumBytesPerSector - 1)) == 0
           && (br->BPB.NumSectorsPerCluster
               & (br->BPB.NumSectorsPerCluster - 1)) == 0
           && br->BPB.NumFATsPresent > 0);
    return out;
}

u64 FileAllocationTableDriver::byte_offset(StorageDeviceDriver* driver, const char* path) {
    (void)path;

    auto buffer = SmartPtr<u8[]>(new u8[512], 512);
    if (buffer.get() == nullptr)
        return -1ull;

    driver->read(0, 512, buffer.get());
    auto* br = reinterpret_cast<BootRecord*>(buffer.get());
    (void)br;

    /* TODO: Next, we probably need to iterate clusters starting at root
     *       , saving subdirectories, checking for file with name of path.
     *       If not found in root, start checking subdirectories, etc.
     */

    SinglyLinkedList<ClusterEntry*> subdirectories;

    return -1ull;
}

u64 FileAllocationTableDriver::root_directory_sectors(BootRecord* br) {
    if (br == nullptr)
        return 0;
        
    return ((br->BPB.NumEntriesInRoot * FAT_DIRECTORY_SIZE_BYTES)
            + (br->BPB.NumBytesPerSector - 1)) / br->BPB.NumBytesPerSector;
}

u64 FileAllocationTableDriver::fat_sectors(BootRecord* br) {
    if (br == nullptr)
        return 0;
        
    if (br->BPB.NumSectorsPerFAT == 0)
        return ((BootRecordExtension32*)br->Extended)->NumSectorsPerFAT;
    else return br->BPB.NumSectorsPerFAT;
}
