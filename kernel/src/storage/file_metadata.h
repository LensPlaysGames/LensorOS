#ifndef LENSOR_OS_FILE_METADATA_H
#define LENSOR_OS_FILE_METADATA_H

#include <integers.h>
#include <storage/storage_device_driver.h>

struct FileMetadata {
    StorageDeviceDriver* DeviceDriver;
    u64 ByteOffset;
};

#endif /* LENSOR_OS_FILE_METADATA_H */
