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
#include <scheduler.h>
#include <storage/file_metadata.h>
#include <system.h>

#include <string_view>
#include <memory>
#include <vector>
#include <format>


#define get_driver_data(meta) static_cast<PipeEnd*>((meta)->driver_data())

void PipeDriver::close(FileMetadata* meta) {
    if (!meta) return;

    auto* pipe = get_driver_data(meta);
    // TODO: if (!pipe) ...

    auto* pipeBuffer = pipe->Buffer;

    if (pipe->End == PipeEnd::READ) {
        if (pipeBuffer->ReadClosed) {
            std::print("[PIPE]:ERROR: Denying attempt to close read end of pipe ({}) as it has already been closed.\n", (void*)pipe);
            return;
        }
        pipeBuffer->ReadClosed = true;
    } else {
        if (pipeBuffer->WriteClosed) {
            std::print("[PIPE]:ERROR: Denying attempt to close write end of pipe ({}) as it has already been closed.\n", (void*)pipe);
            return;
        }
        pipeBuffer->WriteClosed = true;
        // Run processes waiting to read from this pipe with a return value
        // indicating EOF.
        for (pid_t pid : pipeBuffer->PIDsWaiting) {
            auto* process = Scheduler::process(pid);
            if (!process) {
                //std::print("[PIPE]: close()  Can't unblock process {}  pipeEnd={} pipeBuffer={}\n", pid, (void*)pipe, (void*)pipeBuffer);
                continue;
            }
            //std::print("[PIPE]: close()  Unblocking process {}  pipeEnd={} pipeBuffer={}\n", pid, (void*)pipe, (void*)pipeBuffer);
            process->unblock(true, -1);
        }
        pipeBuffer->PIDsWaiting.clear();
    }
    //std::print("[PIPE]: close()  Freeing {} pipe end at {}  pipeBuffer={}\n", pipe->End == PipeEnd::READ ? "read" : "write", (void*)pipe, (void*)pipeBuffer);
    delete pipe;

    // Only attempt to actually free the underlying pipe buffer if both ends are closed.
    if (!pipeBuffer->ReadClosed || !pipeBuffer->WriteClosed) {
        //std::print("[PIPE]: close()  NOT freeing pipe buffer at {} because both ends are not closed\n", (void*)pipeBuffer);
        return;
    }

    //std::print("[PIPE]: close()  Freeing pipe buffer at {}\n", (void*)pipeBuffer);

    // Remove pipe from named pipe buffer vector.
    auto existing = std::find_if(PipeBuffers.begin(), PipeBuffers.end(), [&meta](const NamedPipeBuffer& existing_pipe) {
        return std::string_view(existing_pipe.name) == meta->name();
    });
    if (existing != PipeBuffers.end()) PipeBuffers.erase(existing);

    if (std::find(FreePipeBuffers.begin(), FreePipeBuffers.end(), pipeBuffer) != FreePipeBuffers.end()) {
        std::print("[PIPE]:ERROR: Denying attempt to free pipe buffer at {} more than once!\n", (void*)pipeBuffer);
        return;
    }

    pipeBuffer->clear();
    FreePipeBuffers.push_back(pipeBuffer);
    //std::print("[PIPE]: Closed pipe buffer at {}\n", (void*)pipeBuffer);
}

auto PipeDriver::open(std::string_view path) -> std::shared_ptr<FileMetadata> {
    // FIXME: All horribly wrong

    panic("TODO: Support opening named pipes");
    hang();

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
    auto* pipe = get_driver_data(meta);
    if (!pipe) {
        std::print("[PIPE]:ERROR: Null pipe (driver_data of FileMetadata)\n");
        return -1;
    }

    //std::print("[PIPE]: Reading from pipe buffer at {}\n", (void*)pipe);

    // If there is nothing to read, we either return EOF or block the
    // process until there is something to read.
    if (pipe->Buffer->Offset == 0) {
        // return EOF when write end of pipe is completely closed.
        if (pipe->Buffer->WriteClosed) {
            //std::print("[PIPE]: read()  Returning EOF because write end is closed and pipe is empty\n");
            return -1;
        }

        auto* process = Scheduler::CurrentProcess->value();
        //std::print("[PIPE]: read()  Blocking process {}  pipeEnd={} pipeBuffer={}\n", process->ProcessID, (void*)pipe, (void*)pipe->Buffer);
        pipe->Buffer->PIDsWaiting.push_back(process->ProcessID);
        return -2;
    }

    // TODO: Read in a loop to fill buffers larger than what is currently written.
    // For now, truncate read if it is too large.
    if (byteCount > pipe->Buffer->Offset) {
        std::print("[PIPE]:WARN: Read too large; truncating...");
        byteCount = pipe->Buffer->Offset;
    }
    // Read data
    memcpy(buffer, pipe->Buffer->Data, byteCount);

    // Ensure pipe data starts at beginning for next read.
    memmove(pipe->Buffer->Data, pipe->Buffer->Data + byteCount, PIPE_BUFSZ - byteCount);
    pipe->Buffer->Offset -= byteCount;

    return ssz(byteCount);
};


ssz PipeDriver::write(FileMetadata* meta, usz, usz byteCount, void* buffer) {
    if (!meta) return -1;
    auto* pipe = get_driver_data(meta);
    if (!pipe) return -1;

    //std::print("[PIPE]: Writing to pipe buffer at {}\n", (void*)pipe);

    if (pipe->Buffer->Offset + byteCount > PIPE_BUFSZ) {
        // TODO: Support "wait if full". For now, just truncate write.
        byteCount = PIPE_BUFSZ - pipe->Buffer->Offset;
    }

    memcpy(pipe->Buffer->Data + pipe->Buffer->Offset, buffer, byteCount);
    pipe->Buffer->Offset += byteCount;

    // Run processes waiting to read from this pipe with a return value
    // indicating that the syscall should be retried.
    for (pid_t pid : pipe->Buffer->PIDsWaiting) {
        auto* process = Scheduler::process(pid);
        if (!process) continue;
        //std::print("[PIPE]: write()  Unblocking process {}  pipeEnd={} pipeBuffer={}\n", pid, (void*)pipe, (void*)pipe->Buffer);
        // Set return value of process to retry syscall.
        process->unblock(true, -2);
    }
    pipe->Buffer->PIDsWaiting.clear();

    return ssz(byteCount);
}

auto PipeDriver::lay_pipe() -> PipeMetas {
    PipeBuffer* pipe = nullptr;
    if (FreePipeBuffers.empty()) {
        pipe = new PipeBuffer();
        //std::print("[PIPE]: Allocated new pipe buffer at {}\n", (void*)pipe);
    } else {
        pipe = FreePipeBuffers.back();
        FreePipeBuffers.pop_back();
        //std::print("[PIPE]: Re-used existing pipe buffer at {}\n", (void*)pipe);
    }

    auto* readEnd = new PipeEnd{pipe, PipeEnd::READ};
    auto* writeEnd = new PipeEnd{pipe, PipeEnd::WRITE};

    // TODO: Just a phony path so that we can tell what is an unnamed pipe in debug printouts.
    static usz counter = 0;
    std::string path = std::format("panon{}", ++counter);

    auto readMeta = std::make_shared<FileMetadata>(path, sdd(SYSTEM->virtual_filesystem().PipesDriver), PIPE_BUFSZ, readEnd);
    auto writeMeta = std::make_shared<FileMetadata>(path, sdd(SYSTEM->virtual_filesystem().PipesDriver), PIPE_BUFSZ, writeEnd);

    //std::print("[PIPE]: lay_pipe()  \"{}\"  buffer={}  read={}  write={}\n", path, (void*)pipe, (void*)readEnd, (void*)writeEnd);

    return {readMeta, writeMeta};
}

#undef get_driver_data
