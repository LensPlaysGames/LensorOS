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

#ifndef LENSOR_OS_FILESYSTEM_DRIVER_H
#define LENSOR_OS_FILESYSTEM_DRIVER_H

#include <storage/storage_device_driver.h>
#include <string.h>

class FileMetadata;

/// An abstraction on top of StorageDeviceDriver that returns metadata
/// and byte offset of a given file path to the VFS.
class FilesystemDriver {
public:
    /// If the storage device contains a valid filesystem, `test()` will
    /// return `true`; if a valid filesystem isn't found, `false` is returned.
    virtual bool test(StorageDeviceDriver* driver) = 0;

    /// Get the metadata for the file at path, if it exists.
    virtual FileMetadata file(StorageDeviceDriver* driver, const String& path) = 0;
};

#endif /* LENSOR_OS_FILESYSTEM_DRIVER_H */
