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

#ifndef LENSOR_OS_STORAGE_DEVICE_DRIVER_H
#define LENSOR_OS_STORAGE_DEVICE_DRIVER_H

#include <integers.h>
#include <pure_virtuals.h>

struct FileMetadata;

struct StorageDeviceDriver {
    virtual ~StorageDeviceDriver() = default;
    virtual void close(FileMetadata* file) = 0;
    virtual auto open(std::string_view path) -> std::shared_ptr<FileMetadata> = 0;
    [[gnu::nonnull(2)]] virtual auto read(FileMetadata* file, usz offs, usz bytes, void* buffer) -> ssz = 0;
    virtual auto read_raw(usz offs, usz bytes, void* buffer) -> ssz = 0;
    virtual auto write(FileMetadata* file, usz offs, usz bytes, void* buffer) -> ssz = 0;
};

/// Helper function to convert a Driver to a StorageDeviceDriver.
template <typename Derived>
auto sdd(Derived&& der) -> std::shared_ptr<StorageDeviceDriver> {
    if constexpr (std::is_same_v<std::remove_cvref_t<Derived>, std::shared_ptr<StorageDeviceDriver>>) return der;
    else return std::static_pointer_cast<StorageDeviceDriver>(std::forward<Derived>(der));
}

#endif /* LENSOR_OS_STORAGE_DEVICE_DRIVER_H */
