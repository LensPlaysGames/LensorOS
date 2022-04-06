#ifndef LENSOR_OS_SYSTEM_H
#define LENSOR_OS_SYSTEM_H

#include "cpu.h"
#include "file.h"
#include "guid.h"
#include "integers.h"
#include "linked_list.h"
#include "memory/physical_memory_manager.h"
#include "pure_virtuals.h"

/* TODO:
 * |-- What to do about duplicate devices? count? unique id?
 * `-- Keep track of clock sources (either through system devices or otherwise)
 */

/*     Storage Dev. Driver                        
 *     |-- read(SectorOffset,SectorCount,Buffer)  
 *     `-- write(SectorOffset,SectorCount,Buffer) 
 *
 *     Filesystem Driver
 *     |-- read(Storage Dev. Driver Ptr,Path,Buffer,Count)
 *     `-- write(Storage Dev. Driver Ptr,Path,Buffer,Count)
 *
 *     Filesystem Driver calls Storage Device Driver
 */

/* System Device Number:
 * |-- 0: Reserved
 * `-- 1: Storage Device -- StorageDeviceDriver at Data1
 *     |-- 0: AHCI Controller
 *     |   `-- Data2: PCI Device Header Address
 *     |-- 1: AHCI Port
 *     |   `-- Data2: AHCI Controller System Device Ptr
 *     `-- 10: GPT Partition
 *         |-- Data2: Partition Data (Sector offset, total size)
 *         `-- Data4: System Device Ptr (To storage device this part. resides on)
 */

constexpr u64 SYSDEV_MAJOR_STORAGE = 1;
constexpr u64 SYSDEV_MINOR_AHCI_CONTROLLER = 0;
constexpr u64 SYSDEV_MINOR_AHCI_PORT = 1;
constexpr u64 SYSDEV_MINOR_GPT_PARTITION = 10;

/// A system device refers to a hardware device that has been detected
/// during boot, and is saved for later use in the system structure.
class SystemDevice {
public:
    SystemDevice(u64 major, u64 minor)
        : Major(major), Minor(minor)
    {
        Data1 = nullptr;
        Data2 = nullptr;
        Data3 = nullptr;
        Data4 = nullptr;
    };

    SystemDevice(u64 major, u64 minor, void* data)
        : Major(major), Minor(minor), Data1(data)
    {
        Data2 = nullptr;
        Data3 = nullptr;
        Data4 = nullptr;
    };

    SystemDevice(u64 major, u64 minor
                 , void* data1, void* data2
                 , void* data3, void* data4)
        : Major(major), Minor(minor)
        , Data1(data1), Data2(data2)
        , Data3(data3), Data4(data4) {};

    u64 major() { return Major; }
    u64 minor() { return Minor; }
    void* data1() { return Data1; }
    void* data2() { return Data2; }
    void* data3() { return Data3; }
    void* data4() { return Data4; }

private:
    u64 Major { 0 };
    u64 Minor { 0 };
    void* Data1 { nullptr };
    void* Data2 { nullptr };
    void* Data3 { nullptr };
    void* Data4 { nullptr };
};

class StorageDeviceDriver {
public:
    virtual void read(u64 byteOffset, u64 byteCount, u8* buffer) = 0;
    virtual void write(u64 byteOffset, u64 byteCount, u8* buffer) = 0;
};

class GPTPartitionDriver final : public StorageDeviceDriver {
public:
    GPTPartitionDriver(StorageDeviceDriver* driver
                       , GUID type, GUID unique
                       , u64 startSector, u64 sectorSize)
        : Driver(driver)
        , Type(type), Unique(unique)
        , Offset(startSector * sectorSize) {}

    virtual void read(u64 byteOffset, u64 byteCount, u8* buffer) override {
        Driver->read(byteOffset + Offset, byteCount, buffer);
    };

    virtual void write(u64 byteOffset, u64 byteCount, u8* buffer) override {
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

class FilesystemDriver {
public:
    virtual void read(StorageDeviceDriver* driver
                      , const char* path
                      , void* buffer, u64 numBytes) = 0;
    virtual void write(StorageDeviceDriver* driver
                       , const char* path
                       , void* buffer, u64 numBytes) = 0;
};

enum class FilesystemType {
    INVALID = 0,
    FAT = 1,
};

static const char* fstype_to_string(FilesystemType t) {
    switch (t) {
    case FilesystemType::INVALID:
        return "Invalid";
    case FilesystemType::FAT:
        return "File Allocation Table";
    default:
        return "Unknown filesystem type";
    };
};

class Filesystem {
public:
    Filesystem(FilesystemType t
               , FilesystemDriver* fs
               , StorageDeviceDriver* dev)
        : Type(t), FSDriver(fs), DevDriver(dev) {}

    FilesystemType type() { return Type; }
    FilesystemDriver* filesystem_driver() { return FSDriver; }
    StorageDeviceDriver* storage_device_driver() { return DevDriver; }

    void read(const char* path, void* buffer, u64 numBytes) {
        FSDriver->read(DevDriver, path, buffer, numBytes);
    };

    void write(const char* path, void* buffer, u64 numBytes) {
        FSDriver->write(DevDriver, path, buffer, numBytes);
    };

private:
    FilesystemType Type; // This doesn't do much right now.
    FilesystemDriver* FSDriver { nullptr };
    StorageDeviceDriver* DevDriver { nullptr };
};

class System {
public:
    System() {}

    void set_cpu(const CPUDescription& cpu) {
        CPU = cpu;
    }

    void add_device(SystemDevice d) {
        Devices.add(d);
    };

    void add_fs(Filesystem fs) {
        Filesystems.add(fs);
    }

    CPUDescription& cpu() { return CPU; }

    SinglyLinkedList<SystemDevice>& devices() { return Devices; }
    SystemDevice* device(u64 major, u64 minor) {
        Devices.for_each([major, minor](auto* it) {
            SystemDevice& dev = it->value();
            if (dev.major() == major && dev.minor() == minor)
                return dev;
        });
        return nullptr;
    }

    void print() {
        CPU.print_debug();
        Devices.for_each([](auto* it){
            SystemDevice& dev = it->value();
            UART::out("System Device ");
            UART::out(dev.major());
            UART::out(".");
            UART::out(dev.minor());
            UART::out(":\r\n  Data1: 0x");
            UART::out(to_hexstring(dev.data1()));
            UART::out("\r\n  Data2: 0x");
            UART::out(to_hexstring(dev.data2()));
            UART::out("\r\n  Data3: 0x");
            UART::out(to_hexstring(dev.data3()));
            UART::out("\r\n  Data4: 0x");
            UART::out(to_hexstring(dev.data4()));
            UART::out("\r\n");
        });
        UART::out("\r\n");
        Filesystems.for_each([](auto* it){
            Filesystem& fs = it->value();
            UART::out("Filesystem: ");
            UART::out(fstype_to_string(fs.type()));
            UART::out("\r\n  Filesystem Driver Address: 0x");
            UART::out(to_hexstring(fs.filesystem_driver()));
            UART::out("\r\n  Storage Device Driver Address: 0x");
            UART::out(to_hexstring(fs.storage_device_driver()));
            UART::out("\r\n");
        });
        UART::out("\r\n");
    }

private:
    CPUDescription CPU;
    SinglyLinkedList<SystemDevice> Devices;
    SinglyLinkedList<Filesystem> Filesystems;
};

extern System* SYSTEM;

#endif /* LENSOR_OS_SYSTEM_H */
