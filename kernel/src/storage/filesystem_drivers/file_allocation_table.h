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

#ifndef LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H
#define LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H

#include <fat_definitions.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <string.h>

class FileAllocationTableDriver final : public FilesystemDriver {
public:
    void print_fat(BootRecord*);

    // ^FilesystemDriver

    /// Return true if the storage device has a valid FAT filesystem.
    bool test (StorageDeviceDriver* driver) final;

    /// Return the byte offset of the contents of a file at a given path.
    FileMetadata file(StorageDeviceDriver* driver, const String& path) final;
};

#endif /* LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H */
