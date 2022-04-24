#ifndef LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H
#define LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H

#include <fat_definitions.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>

class FileAllocationTableDriver final : public FilesystemDriver {
public:
    /// Return true if the storage device has a valid FAT filesystem.
    bool test (StorageDeviceDriver* driver) final;
    u64 byte_offset(StorageDeviceDriver* driver, const char* path) final;

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

    u64 root_directory_sectors(BootRecord*);
    u64 fat_sectors(BootRecord*);
};

#endif /* LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H */
