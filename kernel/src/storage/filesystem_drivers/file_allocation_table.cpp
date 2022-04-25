#include <storage/filesystem_drivers/file_allocation_table.h>

#include <debug.h>
#include <fat_definitions.h>
#include <integers.h>
#include <linked_list.h>
#include <smart_pointer.h>
#include <string.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_FAT

void FileAllocationTableDriver::print_fat(BootRecord* br) {
    if (br == nullptr)
        return;

    dbgmsg("File Allocation Table Boot Record:\r\n"
           "  Total Clusters:      %ull\r\n"
           "  Sectors / Cluster:   %hhu\r\n"
           "  Total Sectors:       %ull\r\n"
           "  Bytes / Sector:      %hu\r\n"
           "  Sectors / FAT:       %ull\r\n"
           "  Sector Offsets:\r\n"
           "    FATs:      %ull\r\n"
           "    Data:      %ull\r\n"
           "    Root Dir.: %ull\r\n"
           "\r\n"
           , br->total_clusters()
           , br->BPB.NumSectorsPerCluster
           , br->BPB.total_sectors()
           , br->BPB.NumBytesPerSector
           , br->fat_sectors()
           , br->BPB.first_fat_sector()
           , br->first_data_sector()
           , br->first_root_directory_sector()
           );
}

bool FileAllocationTableDriver::test(StorageDeviceDriver* driver) {
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

#ifdef DEBUG_FAT
    print_fat(br);
#endif /* DEBUG_FAT */

    return out;
}

u64 FileAllocationTableDriver::byte_offset(StorageDeviceDriver* driver, const char* path) {
    (void)driver;
    (void)path;
    return -1ull;
}
