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

#ifndef LENSOR_OS_INPUT_DRIVER_H
#define LENSOR_OS_INPUT_DRIVER_H

#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <memory/common.h>
#include <storage/filesystem_driver.h>
#include <storage/file_metadata.h>
#include <scheduler.h>

// NOTE: This is an attempt to keep `sizeof(InputBuffer)` == PAGE_SIZE
#define INPUT_BUFSZ PAGE_SIZE - sizeof(usz) - sizeof(std::vector<pid_t>)

struct InputBuffer {
    u8 Data[INPUT_BUFSZ];
    usz Offset{};
    std::vector<pid_t> PIDsWaiting{};

    constexpr InputBuffer() = default;
    ~InputBuffer() = default;

    /// Copying an input buffer is nonsense.
    InputBuffer(const InputBuffer&) = delete;

    /// We don’t allow moving either so we don’t accidentally move an
    /// input buffer while someone is reading from or writing to it.
    InputBuffer(InputBuffer&&) = delete;
};

struct NamedInputBuffer {
    std::weak_ptr<FileMetadata> Meta;
    std::string Name;
    InputBuffer* Buffer;
};

struct InputDriver final : FilesystemDriver {
    void close(FileMetadata* file) final;
    std::shared_ptr<FileMetadata> open(std::string_view path) final;

    ssz read_raw(usz, usz, void*) final { return -1; }
    ssz flush(FileMetadata* file) final { return -1; };

    ssz read(FileMetadata* file, usz, usz bytes, void* buffer) final;
    ssz write(FileMetadata* file, usz, usz bytes, void* buffer) final;


    auto device() -> std::shared_ptr<StorageDeviceDriver> final {
        return {};
    };
    auto name() -> const char* final {
        return "Input";
    };

private:
    std::vector<NamedInputBuffer> InputBuffers;
    std::vector<InputBuffer*> FreeInputBuffers;
};

#endif /* LENSOR_OS_INPUT_DRIVER_H */
