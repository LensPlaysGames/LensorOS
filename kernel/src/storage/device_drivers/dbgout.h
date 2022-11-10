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

#ifndef LENSOR_OS_DBGOUT_DRIVER_H
#define LENSOR_OS_DBGOUT_DRIVER_H

#include <debug.h>
#include <integers.h>
#include <storage/storage_device_driver.h>

struct DbgOutDriver final : StorageDeviceDriver {
    /// Required by the interface but not valid for this driver.
    void close(FileMetadata*) final { }
    auto open(std::string_view) -> std::shared_ptr<FileMetadata> final
        { return std::shared_ptr<FileMetadata>{nullptr}; }

    ssz read(FileMetadata*, usz, usz, void*) final { return -1; };
    ssz write(FileMetadata*, usz offs, usz bytes, void* buffer) final {
        dbgmsg_buf(reinterpret_cast<u8*>(buffer) + offs, bytes);
        return ssz(bytes);
    };
};

#endif /* LENSOR_OS_DBGOUT_DRIVER_H */
