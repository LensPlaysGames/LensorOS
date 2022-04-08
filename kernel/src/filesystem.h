#ifndef LENSOR_OS_FILESYSTEM_H
#define LENSOR_OS_FILESYSTEM_H

#include "storage/filesystem_driver.h"
#include "storage/storage_device_driver.h"

enum class FilesystemType {
    INVALID = 0,
    FAT = 1,
};

class Filesystem {
    /* TODO:
     * `-- The public API is not what is required of this class.
     *     Needs to better support open, then read/write, then close.
     */
public:
    Filesystem(FilesystemType t
               , FilesystemDriver* fs
               , StorageDeviceDriver* dev)
        : Type(t), FSDriver(fs), DevDriver(dev) {}

    static const char* type2name(FilesystemType t) {
        switch (t) {
        case FilesystemType::INVALID:
            return "Invalid";
        case FilesystemType::FAT:
            return "File Allocation Table";
        default:
            return "Unkown Filesystem Type";
        }
    }

    FilesystemType type() { return Type; }
    FilesystemDriver* filesystem_driver() { return FSDriver; }
    StorageDeviceDriver* storage_device_driver() { return DevDriver; }

    void read(const char* path, void* buffer, u64 numBytes) {
        FSDriver->read(DevDriver, path, buffer, numBytes);
    };

    void write(const char* path, void* buffer, u64 numBytes) {
        FSDriver->write(DevDriver, path, buffer, numBytes);
    };

    void print() {
        UART::out("Filesystem: ");
        UART::out(type2name(Type));
        UART::out("\r\n"
                  "  Filesystem Driver Address: 0x");
        UART::out(to_hexstring(FSDriver));
        UART::out("\r\n"
                  "  Storage Device Driver Address: 0x");
        UART::out(to_hexstring(DevDriver));
        UART::out("\r\n");
    }

private:
    FilesystemType Type { FilesystemType::INVALID };
    FilesystemDriver* FSDriver { nullptr };
    StorageDeviceDriver* DevDriver { nullptr };
};

#endif /* LENSOR_OS_FILESYSTEM_H */
