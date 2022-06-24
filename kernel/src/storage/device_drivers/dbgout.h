#ifndef LENSOR_OS_DBGOUT_DRIVER_H
#define LENSOR_OS_DBGOUT_DRIVER_H

#include <debug.h>
#include <integers.h>
#include <storage/storage_device_driver.h>

class DbgOutDriver final : public StorageDeviceDriver {
public:
    void read(u64 byteOffset, u64 byteCount, u8* buffer) {
        (void)byteOffset;
        (void)byteCount;
        (void)buffer;
        return;
    };
    void write(u64 byteOffset, u64 byteCount, u8* buffer) {
        dbgmsg_buf(buffer + byteOffset, byteCount);
    };
};

#endif /* LENSOR_OS_DBGOUT_DRIVER_H */
