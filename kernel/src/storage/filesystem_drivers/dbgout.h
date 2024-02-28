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
#include <storage/filesystem_driver.h>

struct DbgOutDriver final : FilesystemDriver {
    ssz read_raw(usz offs, usz bytes, void* buffer) final {
        return -1;
    };
    ssz read(FileMetadata* file, usz offset, usz size, void* buffer) final {
        return -1;
    }
    ssz flush(FileMetadata* file) final {
        return -1;
    };
    std::shared_ptr<FileMetadata> open(std::string_view path) final {
        return {};
    };
    void close(FileMetadata* file) final {
        return;
    };

    ssz write(FileMetadata* file, usz offset, usz size, void* buffer) final {
        dbgmsg_buf(reinterpret_cast<u8*>(buffer) + offset, size);
        return ssz(size);
    };

    auto device() -> std::shared_ptr<StorageDeviceDriver> final {
        return nullptr;
    };
    auto name() -> const char* final {
        return "DbgOut";
    };
};

#endif /* LENSOR_OS_DBGOUT_DRIVER_H */
