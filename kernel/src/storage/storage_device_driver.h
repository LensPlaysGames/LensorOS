#ifndef LENSOR_OS_STORAGE_DEVICE_DRIVER_H
#define LENSOR_OS_STORAGE_DEVICE_DRIVER_H

#include "../integers.h"
#include "../pure_virtuals.h"

class StorageDeviceDriver {
public:
    virtual void read(u64 byteOffset, u64 byteCount, u8* buffer) = 0;
    virtual void write(u64 byteOffset, u64 byteCount, u8* buffer) = 0;
};

#endif /* LENSOR_OS_STORAGE_DEVICE_DRIVER_H */
