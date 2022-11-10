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

#ifndef LENSOR_OS_FILESYSTEM_H
#define LENSOR_OS_FILESYSTEM_H

#include <format>
#include <vector>

#include <debug.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>

/*

enum class FilesystemType {
    INVALID = 0,
    FAT = 1,
};
*/

/*class Filesystem {
    *//* TODO:
     * `-- The public API is not what is required of this class.
     *     Needs to better support open, then read/write, then close.
     *//*
    /// Required by std::vector.
    Filesystem() = default;
    friend std::vector<Filesystem>;
public:

    Filesystem(FilesystemType t, StorageDeviceDriver* dev)
        : Type(t), DevDriver(dev) {}



    FilesystemType type() { return Type; }
    StorageDeviceDriver* storage_device_driver() { return DevDriver; }

    void print() {
        std::print("Filesystem: {}\n"
                   "  Storage Device Driver Address: {}\n"
                   , type2name(Type)
                   , (void*) DevDriver
                   );
    }

private:
    FilesystemType Type { FilesystemType::INVALID };
    StorageDeviceDriver* DevDriver { nullptr };
};*/

#endif /* LENSOR_OS_FILESYSTEM_H */
