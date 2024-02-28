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

#include <storage/filesystem_drivers/input.h>

#include <storage/file_metadata.h>
#include <system.h>
#include <scheduler.h>
#include <virtual_filesystem.h>

#include <string_view>
#include <memory>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_INPUT_DRIVER

#ifdef DEBUG_INPUT_DRIVER
# define DBGMSG(...) std::print(__VA_ARGS__)
#else
# define DBGMSG(...)
#endif

void InputDriver::close(FileMetadata* file) {
    if (!file) return;
    auto* input = static_cast<InputBuffer*>(file->driver_data());
    FreeInputBuffers.push_back(input);
}

std::shared_ptr<FileMetadata> InputDriver::open(std::string_view path) {
    // FIXME: We may want to disallow empty paths.

    for (const auto& existing_buffer : InputBuffers)
        if (std::string_view(existing_buffer.Name) == path)
            return existing_buffer.Meta.lock();

    InputBuffer* input = nullptr;
    if (FreeInputBuffers.empty()) {
        input = new InputBuffer();
    } else {
        input = FreeInputBuffers.back();
        FreeInputBuffers.pop_back();
    }
    auto f = std::make_shared<FileMetadata>(FileMetadata::FileType::Regular, path, fsd(SYSTEM->virtual_filesystem().StdinDriver), INPUT_BUFSZ, input);
    InputBuffers.push_back({f, path, input});
    return f;
}

ssz InputDriver::read(FileMetadata* file, usz, usz bytes, void* buffer) {
    if (!file) return -1;

    auto* input = static_cast<InputBuffer*>(file->driver_data());
    if (!input) return -1;

    // Block until there is something to read.
    if (input->Offset == 0) {
        auto* process = Scheduler::CurrentProcess->value();
        DBGMSG("[INPUT]:  read()  Blocking process {}  buffer at {} has no data\n", process->ProcessID, (void*)input);
        input->PIDsWaiting.push_back(process->ProcessID);
        return -2;
    }

    // TODO: Read in a loop to fill buffers larger than what is currently written.
    // For now, truncate read if it is too large.
    if (bytes > input->Offset) {
        bytes = input->Offset;
    }

    memcpy(buffer, input->Data, bytes);

    // "Pop" bytes read off beginning of buffer.
    // Only on the heap to prevent stack overflow.
    auto temp = new u8[INPUT_BUFSZ];
    memcpy(temp, input->Data, INPUT_BUFSZ);
    memcpy(input->Data, temp + bytes, INPUT_BUFSZ - bytes);
    delete[] temp;

    // Set write offset back, as we have just removed from the beginning.
    input->Offset -= bytes;

    return ssz(bytes);
}

ssz InputDriver::write(FileMetadata* file, usz, usz bytes, void* buffer) {
    if (!file) return -1;
    auto* input = static_cast<InputBuffer*>(file->driver_data());
    if (!input) return -1;
    if (input->Offset + bytes > INPUT_BUFSZ) {
        // TODO: Support "wait if full". For now, just truncate write.
        bytes = INPUT_BUFSZ - input->Offset;
    }
    memcpy(input->Data + input->Offset, buffer, bytes);
    input->Offset += bytes;

    // Run processes waiting for something to happen to this input buffer.
    // Run processes waiting to read from this pipe with a return value
    // indicating that the syscall should be retried.
    for (pid_t pid : input->PIDsWaiting) {
        auto* process = Scheduler::process(pid);
        if (!process) continue;
        DBGMSG("[INPUT]:  write()  Unblocking process {}  input buffer at {}\n", pid, (void*)input);
        // Set return value of process to retry syscall.
        process->unblock(true, -2);
    }
    input->PIDsWaiting.clear();

    return ssz(bytes);
}
