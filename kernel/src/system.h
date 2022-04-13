#ifndef LENSOR_OS_SYSTEM_H
#define LENSOR_OS_SYSTEM_H

#include <cpu.h>
#include <fat_definitions.h>
#include <file.h>
#include <filesystem.h>
#include <guid.h>
#include <integers.h>
#include <linked_list.h>
#include <memory/physical_memory_manager.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>

/* TODO:
 * |-- What to do about duplicate devices? count? unique id?
 * `-- Keep track of clock sources (either through system devices or otherwise)
 *
 * System Device Number:
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

constexpr u64 SYSDEV_MAJOR_STORAGE          = 1;

constexpr u64 SYSDEV_MINOR_AHCI_CONTROLLER  = 0;
constexpr u64 SYSDEV_MINOR_AHCI_PORT        = 1;
constexpr u64 SYSDEV_MINOR_GPT_PARTITION    = 10;

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
        Flags = 0;
    };

    SystemDevice(u64 major, u64 minor, void* data)
        : Major(major), Minor(minor), Data1(data)
    {
        Data2 = nullptr;
        Data3 = nullptr;
        Data4 = nullptr;
        Flags = 0;
    };

    SystemDevice(u64 major, u64 minor
                 , void* data1, void* data2
                 , void* data3, void* data4)
        : Major(major), Minor(minor)
        , Data1(data1), Data2(data2)
        , Data3(data3), Data4(data4)
    {
        Flags = 0;
    };

    void set_flag(u64 flag) {
        Flags |= (1 << flag);
    }

    u64 flag(u64 flag) {
        return Flags & (1 << flag);
    }

    u64 flags() { return Flags; }
    u64 major() { return Major; }
    u64 minor() { return Minor; }
    void* data1() { return Data1; }
    void* data2() { return Data2; }
    void* data3() { return Data3; }
    void* data4() { return Data4; }

private:
    u64 Flags { 0 };
    u64 Major { 0 };
    u64 Minor { 0 };
    void* Data1 { nullptr };
    void* Data2 { nullptr };
    void* Data3 { nullptr };
    void* Data4 { nullptr };
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
        if (Devices.length() > 0) {
            UART::out("System Devices:\r\n");
            Devices.for_each([](auto* it){
                SystemDevice& dev = it->value();
                UART::out("  ");
                UART::out(dev.major());
                UART::out(".");
                UART::out(dev.minor());
                void* d1 = dev.data1();
                void* d2 = dev.data2();
                void* d3 = dev.data3();
                void* d4 = dev.data4();
                if (d1) {
                    UART::out(":\r\n"
                              "    Data1: 0x");
                    UART::out(to_hexstring(d1));
                }
                if (d2) {
                    UART::out("\r\n"
                              "    Data2: 0x");
                    UART::out(to_hexstring(d2));
                }
                if (d3) {
                    UART::out("\r\n"
                              "    Data3: 0x");
                    UART::out(to_hexstring(d3));
                }
                if (d4) {
                    UART::out("\r\n"
                              "    Data4: 0x");
                    UART::out(to_hexstring(d4));
                }
                UART::out("\r\n");
            });
            UART::out("\r\n");
        }
        if (Filesystems.length() > 0) {
            UART::out("Filesystems:\r\n");
            Filesystems.for_each([](auto* it){
                Filesystem& fs = it->value();
                fs.print();
                UART::out("  First 8 bytes: ");
                u64 buffer { 0 };
                fs.storage_device_driver()->read(0, 8, (u8*)&buffer);
                UART::out((u8*)&buffer, 8);
                UART::out("\r\n");
            });
            UART::out("\r\n");
        }
    }

private:
    CPUDescription CPU;
    SinglyLinkedList<SystemDevice> Devices;
    SinglyLinkedList<Filesystem> Filesystems;
};

extern System* SYSTEM;

#endif /* LENSOR_OS_SYSTEM_H */
