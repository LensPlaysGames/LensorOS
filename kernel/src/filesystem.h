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

#include <debug.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>

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

    void print() {
        dbgmsg("Filesystem: %s\r\n"
               "  Filesystem Driver Address: %x\r\n"
               "  Storage Device Driver Address: %x\r\n"
               , type2name(Type)
               , FSDriver
               , DevDriver
               );
    }

private:
    FilesystemType Type { FilesystemType::INVALID };
    FilesystemDriver* FSDriver { nullptr };
    StorageDeviceDriver* DevDriver { nullptr };
};

#endif /* LENSOR_OS_FILESYSTEM_H */
