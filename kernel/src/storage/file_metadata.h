#ifndef LENSOR_OS_FILE_METADATA_H
#define LENSOR_OS_FILE_METADATA_H

#include <integers.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <string.h>

class FileMetadata {
public:
    FileMetadata()
        : Name(""), Invalid(true)
        , DeviceDriver(nullptr)
        , FileDriver(nullptr)
        , ByteOffset(-1ull) {}

    FileMetadata(const String& name, bool invalid
                 , StorageDeviceDriver* deviceDriver
                 , FilesystemDriver* filesystemDriver
                 , u64 byteOffset)
        : Name(name), Invalid(invalid)
        , DeviceDriver(deviceDriver)
        , FileDriver(filesystemDriver)
        , ByteOffset(byteOffset) {}

    String name()                        { return Name;         }
    bool invalid()                       { return Invalid;      }
    StorageDeviceDriver* device_driver() { return DeviceDriver; }
    FilesystemDriver* file_driver()      { return FileDriver;   }
    u64 byte_offset()                    { return ByteOffset;   }

private:
    String Name = { "" };
    bool Invalid { false };
    StorageDeviceDriver* DeviceDriver { nullptr };
    FilesystemDriver* FileDriver { nullptr };
    u64 ByteOffset { -1ull };
};

#endif /* LENSOR_OS_FILE_METADATA_H */
