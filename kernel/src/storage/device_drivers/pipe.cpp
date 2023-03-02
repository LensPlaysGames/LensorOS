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

#include <storage/device_drivers/pipe.h>

#include <vfs_forward.h>
#include <storage/file_metadata.h>
#include <system.h>

#include <string_view>
#include <memory>
#include <vector>
#include <format>


void PipeDriver::close(FileMetadata* meta) {
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
    //std::print("[PIPE]: Closed pipe buffer at {}\n", (void*)pipe);
}

auto PipeDriver::open(std::string_view path) -> std::shared_ptr<FileMetadata> {
    PipeBuffer* pipe = nullptr;
    for (auto& existing_pipe : PipeBuffers)
        if (std::string_view(existing_pipe.name) == path)
            return existing_pipe.meta.lock();

    if (FreePipeBuffers.empty()) {
        pipe = new PipeBuffer();
        //std::print("[PIPE]: Allocated new pipe buffer at {}\n", (void*)pipe);
    } else {
        pipe = FreePipeBuffers.back();
        FreePipeBuffers.pop_back();
        //std::print("[PIPE]: Re-used existing pipe buffer at {}\n", (void*)pipe);
    }
    auto meta = std::make_shared<FileMetadata>(path, sdd(SYSTEM->virtual_filesystem().PipesDriver), PIPE_BUFSZ, pipe);
    PipeBuffers.push_back({meta, path, pipe});
    return meta;
}

ssz PipeDriver::read(FileMetadata* meta, usz, usz byteCount, void* buffer) {
    if (!meta) {
        std::print("[PIPE]:ERROR: Cannot read file given null file metadata\n");
        return -1;
    }
    auto* pipe = static_cast<PipeBuffer*>(meta->driver_data());
    if (!pipe) {
        std::print("[PIPE]:ERROR: Null pipe (driver_data of FileMetadata)\n");
        return -1;
    }

    //std::print("[PIPE]: Reading from pipe buffer at {}\n", (void*)pipe);

    // TODO: Support "wait until there is something to read".
    // I'm hesitant to just stick a while loop here because most
    // likely we are in a syscall and no other processes are actually
    // running...
    if (pipe->Offset == 0) {
        //std::print("[PIPE]:TODO: Support \"wait until there is something to read\"\n");
        return -1;
    }

    // TODO: Read in a loop to fill buffers larger than what is currently written.
    // For now, truncate read if it is too large.
    if (byteCount > pipe->Offset)
        byteCount = pipe->Offset;

    // Read data
    memcpy(buffer, pipe->Data, byteCount);

    // Ensure pipe data starts at beginning for next read.
    memmove(pipe->Data, pipe->Data + byteCount, PIPE_BUFSZ - byteCount);
    pipe->Offset -= byteCount;

    return ssz(byteCount);
};


ssz PipeDriver::write(FileMetadata* meta, usz, usz byteCount, void* buffer) {
    if (!meta) return -1;
    auto* pipe = static_cast<PipeBuffer*>(meta->driver_data());
    if (!pipe) return -1;

    //std::print("[PIPE]: Writing to pipe buffer at {}\n", (void*)pipe);

    if (pipe->Offset + byteCount > PIPE_BUFSZ) {
        // TODO: Support "wait if full". For now, just truncate write.
        byteCount = PIPE_BUFSZ - pipe->Offset;
    }

    memcpy(pipe->Data + pipe->Offset, buffer, byteCount);
    pipe->Offset += byteCount;
    return ssz(byteCount);
}

auto PipeDriver::lay_pipe() -> std::shared_ptr<FileMetadata> {
    static usz counter = 0;
    std::string name = std::format("p{}", counter++);
    return open(name);
}
