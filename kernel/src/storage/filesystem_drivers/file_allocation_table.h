#ifndef LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H
#define LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H

#include <fat_definitions.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <string.h>

class FileAllocationTableDriver final : public FilesystemDriver {
public:
    void print_fat(BootRecord*);

    // ^FilesystemDriver

    /// Return true if the storage device has a valid FAT filesystem.
    bool test (StorageDeviceDriver* driver) final;

    /// Return the byte offset of the contents of a file at a given path.
    u64 byte_offset(StorageDeviceDriver* driver, const String& path) final;
};

#endif /* LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H */
