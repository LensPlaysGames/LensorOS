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

struct FileMetadata {
    FileMetadata()
        : Name(""), Invalid(true)
        , DeviceDriver(nullptr)
        , FileSize(-1ull)
        , DriverData(nullptr) {}

    FileMetadata(std::string name
                 , std::shared_ptr<StorageDeviceDriver> dev_driver
                 , u64 file_size
                 , void* driver_data
                 )
        : Name(std::move(name)), Invalid(false)
        , DeviceDriver(std::move(dev_driver))
        , FileSize(file_size)
        , DriverData(driver_data) {}

    ~FileMetadata() {
        //std::print("Closing FileMetadata \"{}\"\n", Name);
        if (DriverData) {
            //std::print("  Closing on device driver...\n");
            DeviceDriver->close(this);
        }
    }

    usz offset { 0 };

    auto name() -> std::string_view { return Name; }
    auto invalid() -> bool { return Invalid; }
    auto device_driver() -> std::shared_ptr<StorageDeviceDriver> { return DeviceDriver; }
    auto file_size() -> u64 { return FileSize; }
    auto driver_data() -> void* { return DriverData; }

private:
    std::string Name;
    bool Invalid = true;
    // The device driver is used for reading and writing from and to
    // the file.
    std::shared_ptr<StorageDeviceDriver> DeviceDriver { nullptr };
    usz FileSize { -1ull };
    // Driver-specific data.
    void* DriverData { nullptr };
};

#endif /* LENSOR_OS_FILE_METADATA_H */
