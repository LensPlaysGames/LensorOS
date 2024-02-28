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

#ifndef LENSOR_OS_FILE_METADATA_H
#define LENSOR_OS_FILE_METADATA_H

#include <integers.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <string>
#include <format>

struct FileMetadata {
    enum class FileType {
        Regular,
        Directory,
        // TODO: More file types (device, etc)
    };

    FileMetadata()
        : Name(""), Invalid(true)
        , DeviceDriver(nullptr)
        , FileSize(-1ull)
        , DriverData(nullptr) {}

                 , std::shared_ptr<StorageDeviceDriver> dev_driver
    FileMetadata(FileType type, std::string name
                 , u64 file_size
                 , void* driver_data
                 )
        , DeviceDriver(std::move(dev_driver))
        : Type(type), Name(std::move(name)), Invalid(false)
        , FileSize(file_size)
        , DriverData(driver_data) {}

    ~FileMetadata() {
        //std::print("Closing FileMetadata \"{}\"\n", Name);
        if (DeviceDriver && DriverData) DeviceDriver->close(this);
    }

    usz offset { 0 };

    auto name() -> std::string_view { return Name; }
    auto invalid() -> bool { return Invalid; }
    auto device_driver() -> std::shared_ptr<StorageDeviceDriver> { return DeviceDriver; }
    auto file_size() -> u64 { return FileSize; }
    auto driver_data() -> void* { return DriverData; }

    bool is_regular() { return Type == FileType::Regular; }
    bool is_directory() { return Type == FileType::Directory; }

private:
    FileType Type { FileType::Regular };
    std::string Name { "" };
    bool Invalid = true;
    // The device driver is used for reading and writing from and to
    // the file.
    std::shared_ptr<StorageDeviceDriver> DeviceDriver { nullptr };
    usz FileSize { -1ull };
    // Driver-specific data.
    void* DriverData { nullptr };
};

struct DirectoryEntry {
    FileMetadata::FileType type;
    char name[256];
};

#endif /* LENSOR_OS_FILE_METADATA_H */
