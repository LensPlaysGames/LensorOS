#ifndef LENSOR_OS_SYSTEM_H
#define LENSOR_OS_SYSTEM_H

#include "cpu.h"
#include "file.h"
#include "integers.h"
#include "linked_list.h"
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
 * `-- 1: Storage Device
 *     `-- 0: AHCI Controller
 *         |-- Data1: Storage Device Driver Pointer
 *         `-- Data1: PCI Device Header Address
 */

constexpr u64 SYSDEV_AHCI_MAJOR = 1;
constexpr u64 SYSDEV_AHCI_MINOR = 0;

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
    virtual void read(u64 sectorOffset, u64 sectorCount, void* buffer) = 0;
    virtual void write(u64 sectorOffset, u64 sectorCount, void* buffer) = 0;
};

class FilesystemDriver {
public:
    // TODO: What if file at path doesn't exist?
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

private:
    FilesystemType Type;
    FilesystemDriver* FSDriver;
    StorageDeviceDriver* DevDriver;
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
