#ifndef LENSOR_OS_FILESYSTEM_DRIVER_H
#define LENSOR_OS_FILESYSTEM_DRIVER_H

#include "storage_device_driver.h"

/// An abstraction on top of StorageDeviceDriver that returns metadata
/// and byte offset of a given file path to the VFS.
class FilesystemDriver {
public:
    /// If the storage device contains a valid filesystem, `test()` will
    /// return `true`; if a valid filesystem isn't found, `false` is returned.
    virtual bool test(StorageDeviceDriver* driver) = 0;
    /// Read from the file at `path` a given
    /// number of bytes `numBytes` into `buffer`.
    virtual void read(StorageDeviceDriver* driver
                      , const char* path
                      , void* buffer, u64 numBytes) = 0;
    /// Write to the file at `path` a given
    /// number of bytes `numBytes` from `buffer`.
    virtual void write(StorageDeviceDriver* driver
                       , const char* path
                       , void* buffer, u64 numBytes) = 0;
};

#endif /* LENSOR_OS_FILESYSTEM_DRIVER_H */
