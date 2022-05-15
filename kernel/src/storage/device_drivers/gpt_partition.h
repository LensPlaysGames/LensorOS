#ifndef LENSOR_OS_GPT_PARTITION_DRIVER_H
#define LENSOR_OS_GPT_PARTITION_DRIVER_H

#include <guid.h>
#include <storage/storage_device_driver.h>

class GPTPartitionDriver final : public StorageDeviceDriver {
public:
    GPTPartitionDriver(StorageDeviceDriver* driver
                       , GUID type, GUID unique
                       , u64 startSector, u64 sectorSize)
        : Driver(driver)
        , Type(type), Unique(unique)
        , Offset(startSector * sectorSize) {}

    void read(u64 byteOffset, u64 byteCount, u8* buffer) final {
        Driver->read(byteOffset + Offset, byteCount, buffer);
    };

    void write(u64 byteOffset, u64 byteCount, u8* buffer) final {
        Driver->read(byteOffset + Offset, byteCount, buffer);
    };

    GUID type_guid() { return Type; }
    GUID unique_guid() { return Unique; }

private:
    StorageDeviceDriver* Driver { nullptr };
    GUID Type;
    GUID Unique;
    /// Number of bytes to offset within storage device for start of partition.
    u64 Offset { 0 };
};

#endif /* LENSOR_OS_GPT_PARTITION_DRIVER_H */
