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
#include <scheduler.h>

#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <format>

#define PIPE_BUFSZ 512

//   pipe
//  o====o
//  r    w
//
// A pipe has a read end and a write end. The write end can push data
// into the pipe. The read end can pop data from the pipe. Pipes follow
// the FIFO principle; first in, first out (just like a pipe in real
// life).

// TODO: Support `open` for FIFOs (named pipes).

struct PipeBuffer {
    u8 Data[PIPE_BUFSZ]{0};
    usz Offset{0};
    bool ReadClosed{false};
    bool WriteClosed{false};
    std::vector<pid_t> PIDsWaitingOnReadToWrite;
    std::vector<pid_t> PIDsWaitingOnWriteToRead;

    constexpr PipeBuffer() = default;
    ~PipeBuffer() = default;

    /// Copying a pipe buffer is nonsense.
    PipeBuffer(const PipeBuffer&) = delete;

    /// We don’t allow moving either so we don’t accidentally move
    /// a pipe buffer while someone is reading from or writing to it.
    PipeBuffer(PipeBuffer&&) = delete;

    void clear() {
        PIDsWaitingOnReadToWrite.clear();
        PIDsWaitingOnWriteToRead.clear();
        memset(&Data[0], 0, sizeof(Data));
        Offset = 0;
        ReadClosed = false;
        WriteClosed = false;
    }
};


struct PipeMetas {
    std::shared_ptr<FileMetadata> Read;
    std::shared_ptr<FileMetadata> Write;

    PipeMetas(std::shared_ptr<FileMetadata> readMeta, std::shared_ptr<FileMetadata> writeMeta)
    : Read(readMeta), Write(writeMeta) {}
};

struct PipeEnd {
    PipeBuffer* Buffer;
    enum EndType{
        READ,
        WRITE,
    } End;

    PipeEnd(PipeBuffer* buffer, PipeEnd::EndType endType)
    : Buffer(buffer), End(endType) {}
};

struct NamedPipeBuffer {
    std::weak_ptr<FileMetadata> meta;
    std::string name;
    PipeBuffer *pipe;
};

struct PipeDriver final : FilesystemDriver {
    void close(FileMetadata* meta) final;
    auto open(std::string_view path) -> std::shared_ptr<FileMetadata> final;

    ssz read(FileMetadata* meta, usz, usz byteCount, void* buffer) final;
    ssz read_raw(usz, usz, void*) final { return -1; };
    ssz write(FileMetadata* meta, usz, usz byteCount, void* buffer) final;

    auto device() -> std::shared_ptr<StorageDeviceDriver> final {
        return {};
    };
    auto name() -> const char* final {
        return "Pipe";
    };

    PipeMetas lay_pipe();

private:
    /// Stores pipe buffers along with names for mock-filesystem functionality.
    std::vector<NamedPipeBuffer> PipeBuffers;
    std::vector<PipeBuffer*> FreePipeBuffers;
};

#endif /* LENSOR_OS_PIPE_DRIVER_H */
