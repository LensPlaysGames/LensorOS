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

#include <integers.h>
#include <storage/storage_device_driver.h>
#include <storage/file_metadata.h>

#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <format>

#define PIPE_BUFSZ 512

struct PipeBuffer {
    u8 Data[PIPE_BUFSZ]{0};
    usz Offset{0};

    constexpr PipeBuffer() = default;
    ~PipeBuffer() = default;

    /// Copying a pipe buffer is nonsense.
    PipeBuffer(const PipeBuffer&) = delete;

    /// We don’t allow moving either so we don’t accidentally move
    /// a pipe buffer while someone is reading from or writing to it.
    PipeBuffer(PipeBuffer&&) = delete;
};

struct NamedPipeBuffer {
    std::weak_ptr<FileMetadata> meta;
    std::string name;
    PipeBuffer *pipe;
};

struct PipeDriver final : StorageDeviceDriver {
    void close(FileMetadata* meta) final {
        if (!meta) return;

        // Remove pipe from named pipe buffer vector.
        for (const NamedPipeBuffer* existing_pipe = PipeBuffers.begin(); existing_pipe != PipeBuffers.end(); existing_pipe++)
            if (std::string_view(existing_pipe->name) == meta->name()) {
                PipeBuffers.erase(existing_pipe);
                break;
            }

        auto* pipe = static_cast<PipeBuffer*>(meta->driver_data());
        if (std::find(FreePipeBuffers.begin(), FreePipeBuffers.end(), pipe) != FreePipeBuffers.end()) {
            std::print("[PIPE]:ERROR: Denying attempt to free pipe buffer at {} more than once!\n", (void*)pipe);
            return;
        }
        FreePipeBuffers.push_back(pipe);
        std::print("[PIPE]: Closed pipe buffer at {}\n", (void*)pipe);
    }

    auto open(std::string_view path) -> std::shared_ptr<FileMetadata> final;

    ssz read(FileMetadata* meta, usz, usz byteCount, void* buffer) final {
        if (!meta) return -1;
        auto* pipe = static_cast<PipeBuffer*>(meta->driver_data());
        if (!pipe) return -1;

        std::print("[PIPE]: Reading from pipe buffer at {}\n", (void*)pipe);

        // TODO: Support "wait until there is something to read".
        // I'm hesitant to just stick a while loop here because most
        // likely we are in a syscall and no other processes are actually
        // running...
        if (pipe->Offset == 0) return -1;

        // TODO: Read in a loop to fill buffers larger than what is currently written.
        // For now, truncate read if it is too large.
        if (byteCount > pipe->Offset)
            byteCount = pipe->Offset;

        memcpy(buffer, pipe->Data, byteCount);
        // TODO: After data is read, shouldn't we reset pipe->Offset?
        return ssz(byteCount);
    };

    ssz read_raw(usz, usz, void*) final { return -1; };

    ssz write(FileMetadata* meta, usz, usz byteCount, void* buffer) final {
        if (!meta) return -1;
        auto* pipe = static_cast<PipeBuffer*>(meta->driver_data());
        if (!pipe) return -1;

        std::print("[PIPE]: Writing to pipe buffer at {}\n", (void*)pipe);

        if (pipe->Offset + byteCount > PIPE_BUFSZ) {
            // TODO: Support "wait if full". For now, just truncate write.
            byteCount = PIPE_BUFSZ - pipe->Offset;
        }

        memcpy(pipe->Data + pipe->Offset, buffer, byteCount);
        // TODO: After data is read, shouldn't we update pipe->Offset?
        return ssz(byteCount);
    }

    /// Return a byte offset that can be used later to find a unique
    /// buffer, allocated by this function.
    auto lay_pipe() -> std::shared_ptr<FileMetadata> {
        // TODO: Pick suitable file name for file metadata.
        static usz counter = 0;
        std::string name = std::format("p{}", counter++);
        return open(name);
    }

private:
    /// Stores pipe buffers along with names for mock-filesystem functionality.
    std::vector<NamedPipeBuffer> PipeBuffers;
    std::vector<PipeBuffer*> FreePipeBuffers;
};

#endif /* LENSOR_OS_PIPE_DRIVER_H */
