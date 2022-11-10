/* Copyright 2022, Contributors To LensorOS.
 * All rights reserved.
 *
 * This file is part of LensorOS.
 *
 * LensorOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LensorOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LensorOS. If not, see <https://www.gnu.org/licenses
 */

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
#include <virtual_filesystem.h>

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

/* STORAGE DEVICE MAJOR NUMBERS */
constexpr u64 SYSDEV_MAJOR_STORAGE = 1;
/* STORAGE DEVICE FLAGS */
constexpr u64 SYSDEV_MAJOR_STORAGE_SEARCH = 0;
/* STORAGE DEVICE MINOR NUMBERS */
constexpr u64 SYSDEV_MINOR_AHCI_CONTROLLER = 0;
constexpr u64 SYSDEV_MINOR_AHCI_PORT       = 1;
constexpr u64 SYSDEV_MINOR_GPT_PARTITION   = 10;

class System;

/// A system device refers to a hardware device that has been detected
/// during boot, and is saved for later use in the system structure.
class SystemDevice {
    /// Default constructor required by std::vector.
    SystemDevice() = default;
    friend std::vector<SystemDevice>;
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

    void set_flag(u64 bitNumber, bool state) {
        Flags &= ~(1 << bitNumber);
        if (state)
            Flags |= (1 << bitNumber);
    }

    bool flag(u64 bitNumber) {
        return (Flags & (1 << bitNumber)) > 0;
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

    void add_device(SystemDevice&& d) { Devices.push_back(std::move(d)); }
    void add_fs(Filesystem&& fs) { Filesystems.push_back(fs); }

    CPUDescription& cpu() { return CPU; }
    VFS& virtual_filesystem() { return VirtualFilesystem; }

    std::vector<Filesystem>& filesystems() { return Filesystems; }

    std::vector<SystemDevice>& devices() { return Devices; }
    SystemDevice* device(u64 major, u64 minor) {
        for (auto& dev : Devices)
            if (dev.major() == major && dev.minor() == minor)
                return &dev;
        return nullptr;
    }

    void print() {
        CPU.print_debug();
        if (!Devices.empty()) {
            std::print("System Devices:\n");
             for (auto& dev : Devices) {
                std::print("  {}.{}:\n"
                           "    Flags: {}"
                           , dev.major()
                           , dev.minor()
                           , dev.flags());

                if (void* d1 = dev.data1()) std::print("\n    Data1: {}", d1);
                if (void* d2 = dev.data2()) std::print("\n    Data2: {}", d2);
                if (void* d3 = dev.data3()) std::print("\n    Data3: {}", d3);
                if (void* d4 = dev.data4()) std::print("\n    Data4: {}", d4);

                std::print("\n");
            }
            std::print("\n");
        }
        if (!Filesystems.empty()) {
            std::print("Filesystems:\n");
            for (auto& fs : Filesystems) {
                fs.print();
                u8 buffer[8]{};
                fs.storage_device_driver()->read(0, sizeof buffer, buffer);
                std::print("  First 8 bytes: {}\n", __s(buffer));
            }
            std::print("\n");
        }
    }

private:
    CPUDescription CPU;
    std::vector<SystemDevice> Devices;
    std::vector<Filesystem> Filesystems;
    VFS VirtualFilesystem;
};

extern System* SYSTEM;

#endif /* LENSOR_OS_SYSTEM_H */
