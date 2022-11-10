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
#include <string.h>

class FileMetadata {
public:
    FileMetadata()
        : Name(""), Invalid(true)
        , DeviceDriver(nullptr)
        , FileDriver(nullptr)
        , FileSize(-1ull)
        , ByteOffset(-1ull) {}

    FileMetadata(const String& name, bool invalid
                 , StorageDeviceDriver* deviceDriver
                 , FilesystemDriver* filesystemDriver
                 , u64 fileSize
                 , u64 byteOffset
                 )
        : Name(name), Invalid(invalid)
        , DeviceDriver(deviceDriver)
        , FileDriver(filesystemDriver)
        , FileSize(fileSize)
        , ByteOffset(byteOffset) {}

    String name()                        { return Name;         }
    bool invalid()                       { return Invalid;      }
    StorageDeviceDriver* device_driver() { return DeviceDriver; }
    FilesystemDriver* file_driver()      { return FileDriver;   }
    u64 file_size()                      { return FileSize;     }
    u64 byte_offset()                    { return ByteOffset;   }

private:
    String Name = { "" };
    bool Invalid { true };
    // The device driver is used for reading and writing from and to
    // the file.
    StorageDeviceDriver* DeviceDriver { nullptr };
    // The filesystem driver is used for opening and closing the file.
    FilesystemDriver* FileDriver { nullptr };
    u64 FileSize   { -1ull };
    u64 ByteOffset { -1ull };
};

#endif /* LENSOR_OS_FILE_METADATA_H */
