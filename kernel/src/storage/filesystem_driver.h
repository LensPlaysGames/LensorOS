#ifndef LENSOR_OS_FILESYSTEM_DRIVER_H
#define LENSOR_OS_FILESYSTEM_DRIVER_H

#include <storage/storage_device_driver.h>
#include <string.h>

/// An abstraction on top of StorageDeviceDriver that returns metadata
/// and byte offset of a given file path to the VFS.
class FilesystemDriver {
public:
    /// If the storage device contains a valid filesystem, `test()` will
    /// return `true`; if a valid filesystem isn't found, `false` is returned.
    virtual bool test(StorageDeviceDriver* driver) = 0;

    /// Get the byte offset for the file at path, if it exists.
    virtual u64 byte_offset(StorageDeviceDriver* driver, const String& path) = 0;
};

#endif /* LENSOR_OS_FILESYSTEM_DRIVER_H */
