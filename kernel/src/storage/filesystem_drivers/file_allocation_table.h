#ifndef LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H
#define LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H

#include "../filesystem_driver.h"

#include "../../fat_definitions.h"
#include "../../integers.h"
#include "../../smart_pointer.h"

class FileAllocationTableDriver final : public FilesystemDriver {
public:
    bool test (StorageDeviceDriver* driver) final {
        if (driver == nullptr)
            return false;

        auto buffer = SmartPtr<u8[]>(new u8[512], 512);
        if (buffer.get() == nullptr)
            return false;

        bool out { false };
        driver->read(0, 512, buffer.get());
        // potential FIXME: Should this be a `reinterpret_cast`?
        //                                   `static_cast`?
        auto* br = (BootRecord*)buffer.get();
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

    void read(StorageDeviceDriver* driver
              , const char* path
              , void* buffer, u64 numBytes) final
    {
        (void)driver;
        (void)path;
        (void)buffer;
        (void)numBytes;
    }
    void write(StorageDeviceDriver* driver
               , const char* path
               , void* buffer, u64 numBytes) final
    {
        (void)driver;
        (void)path;
        (void)buffer;
        (void)numBytes;        
    }
};

#endif /* LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H */
