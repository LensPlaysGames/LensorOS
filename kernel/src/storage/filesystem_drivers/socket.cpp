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

#include <storage/filesystem_drivers/socket.h>

#include <scheduler.h>
#include <storage/file_metadata.h>
#include <storage/storage_device_driver.h>
#include <system.h>

#include <memory>


std::shared_ptr<FileMetadata> SocketDriver::open(std::string_view path) {
    SocketData* data = new SocketData;
    if (!data) return {};
    // FIXME: We aren't *entirely* sure that the "current process" is
    // the one opening things. Only way to fix this is to pass Process/
    // PID as an argument to every single storage device driver, which is
    // probably a good idea honestly.
    data->PID = Scheduler::CurrentProcess->value()->ProcessID;
    // NOTE: File size has to be non-zero to make sure we don't return early
    // from `read` syscall or any other similar file size checks.
    return FileMetadata::Make(FileMetadata::FileType::Regular, "new_socket", fsd(SYSTEM->virtual_filesystem().SocketsDriver), SOCKET_RX_BUFFER_SIZE, data);
}

void SocketDriver::close(FileMetadata* meta) {
    if (!meta) return;
    SocketData* data = (SocketData*)meta->driver_data();
    if (!data) return;

    switch (data->Type) {
    case SocketType::LENSOR: {
        std::print("  LENSOR type socket; decrementing buffers refcount\n");
        SocketBuffers* buffers = (SocketBuffers*)data->Data;
        if (!buffers) break;
        data->Data = nullptr;
        // FIXME: If we `dup` a socket fd, we're going to have a bad time.
        // Or if we `fork` while one is open; that may also get sketchy.
        buffers->RefCount -= 1;
        if (buffers->RefCount == 0) {
            std::print("  Unbinding and deleting SocketBuffers at {}\n", (void*)buffers);
            // If bound, remove binding for this socket from list of bindings.
            if (data->Address.Type != SocketAddress::UNBOUND) {
                const SocketAddress& addr = data->Address;
                std::erase_if(Bindings, [&addr](const SocketBinding& binding) {
                    return binding == addr;
                });
            }
            delete buffers;
        }
    } break;
    }
    delete data;
}

ssz SocketDriver::read(FileMetadata* meta, usz, usz byteCount, void* buffer) {
    if (!meta) return -1;
    SocketData* data = (SocketData*)meta->driver_data();
    if (!data) return -1;
    switch (data->Type) {
    case SocketType::LENSOR: {
        SocketBuffers* buffers = (SocketBuffers*)data->Data;
        if (!buffers) return -1;
        switch (data->ClientServer) {
        case SocketData::CLIENT:
            return buffers->TXBuffer.read(Scheduler::CurrentProcess->value()->ProcessID, byteCount, (u8*)buffer);
        case SocketData::SERVER:
            return buffers->RXBuffer.read(Scheduler::CurrentProcess->value()->ProcessID, byteCount, (u8*)buffer);
        }
        UNREACHABLE();
    }
    }
    UNREACHABLE();
}

ssz SocketDriver::write(FileMetadata* meta, usz, usz byteCount, void* buffer) {
    if (!meta) return -1;
    SocketData* data = (SocketData*)meta->driver_data();
    if (!data) return -1;
    switch (data->Type) {
    case SocketType::LENSOR: {
        SocketBuffers* buffers = (SocketBuffers*)data->Data;
        if (!buffers) return -1;
        switch (data->ClientServer) {
        case SocketData::CLIENT:
            return buffers->RXBuffer.write(Scheduler::CurrentProcess->value()->ProcessID, byteCount, (u8*)buffer);
        case SocketData::SERVER:
            return buffers->TXBuffer.write(Scheduler::CurrentProcess->value()->ProcessID, byteCount, (u8*)buffer);
        }
        UNREACHABLE();
    }
    }
    return -1;
}

auto SocketDriver::socket(SocketType domain, int type, int protocol) -> std::shared_ptr<FileMetadata> {
    switch (domain) {
    case SocketType::LENSOR: {
        auto f = open("");
        if (!f) return {};
        SocketData* data = (SocketData*)f->driver_data();
        data->Data = new SocketBuffers;
        return f;
    }
    }
    return {};
}
