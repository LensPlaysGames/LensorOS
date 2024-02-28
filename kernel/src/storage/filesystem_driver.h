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
#include <string>

struct FilesystemDriver : StorageDeviceDriver {
    virtual ssz read(FileMetadata* file, usz offs, usz size, void* buffer) = 0;
    virtual ssz write(FileMetadata* file, usz offset, usz size, void* buffer) = 0;
    virtual ssz flush(FileMetadata* file) = 0;

    virtual auto device() -> std::shared_ptr<StorageDeviceDriver> = 0;
    virtual auto name() -> const char* = 0;
};

/// Helper function to convert a Driver to a FilesystemDriver.
template <typename Derived>
auto fsd(Derived&& derived) -> std::shared_ptr<FilesystemDriver> {
    if constexpr (std::is_same_v<std::remove_cvref_t<Derived>, std::shared_ptr<FilesystemDriver>>)
        return derived;
    else return std::static_pointer_cast<FilesystemDriver>(std::forward<Derived>(derived));
}

#endif /* LENSOR_OS_FILESYSTEM_DRIVER_H */
