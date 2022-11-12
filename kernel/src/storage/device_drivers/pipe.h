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

#ifndef LENSOR_OS_PIPE_DRIVER_H
#define LENSOR_OS_PIPE_DRIVER_H

#include <debug.h>
#include <integers.h>
#include <storage/storage_device_driver.h>

#define PIPE_BUFSZ 512

struct PipeBuffer {
    u8 Data[PIPE_BUFSZ]{};
    usz Offset{};

    constexpr PipeBuffer() = default;
    ~PipeBuffer() = default;

    /// Copying a pipe buffer is nonsense.
    PipeBuffer(const PipeBuffer&) = delete;

    /// We don’t allow moving either so we don’t accidentally move
    /// a pipe buffer while someone is reading from or writing to it.
    PipeBuffer(PipeBuffer&&) = delete;
};

struct PipeDriver final : StorageDeviceDriver {
    void close(FileMetadata* meta) final {
        if (!meta) return;
        auto* pipe = static_cast<PipeBuffer*>(meta->driver_data());
        FreePipeBuffers.push_back(pipe);
    }

    auto open(std::string_view path) -> std::shared_ptr<FileMetadata> final;

    ssz read(FileMetadata* file, usz, usz byteCount, void* buffer) final {
        // Find which pipe by using byte offset.
        if (!file) return -1;
        auto* pipe = static_cast<PipeBuffer*>(file->driver_data());
        if (!pipe) return -1;

        // TODO: Support "wait until there is something to read".
        if (pipe->Offset == 0) {
            return -1;
        }

        // TODO: Read in a loop to fill buffers larger than what is currently written.
        if (byteCount > pipe->Offset) {
            byteCount = pipe->Offset;
        }

        memcpy(buffer, pipe->Data + pipe->Offset, byteCount);
        return ssz(byteCount);
    };

    ssz read_raw(usz, usz, void*) final { return -1; };

    ssz write(FileMetadata* file, usz, usz byteCount, void* buffer) final {
        // Find which pipe by using byte offset. There is no filesystem
        // backing, so we can just use that as an index/key to find
        // which buffer to write to.
        if (!file) return -1;
        auto* pipe = static_cast<PipeBuffer*>(file->driver_data());
        if (!pipe) return -1;
        if (pipe->Offset + byteCount > PIPE_BUFSZ) {
            // TODO: Support "wait if full". For now, just truncate write.
            byteCount = PIPE_BUFSZ - pipe->Offset;
        }

        memcpy(pipe->Data + pipe->Offset, buffer, byteCount);
        return ssz(byteCount);
    }

    /// Return a byte offset that can be used later to find a unique
    /// buffer, allocated by this function.
    auto lay_pipe() -> std::shared_ptr<FileMetadata> {
        // TODO: Pick suitable file name for file metadata.
        return open("");
    }

private:
    std::vector<PipeBuffer*> FreePipeBuffers;
};

#endif /* LENSOR_OS_PIPE_DRIVER_H */
