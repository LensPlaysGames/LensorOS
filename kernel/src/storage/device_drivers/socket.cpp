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

#include <storage/device_drivers/socket.h>

#include <scheduler.h>
#include <storage/file_metadata.h>
#include <storage/storage_device_driver.h>
#include <system.h>

#include <memory>


std::shared_ptr<FileMetadata> SocketDriver::open(std::string_view path) {
    SocketData* data = new SocketData;
    return std::make_shared<FileMetadata>("new_socket", sdd(SYSTEM->virtual_filesystem().SocketsDriver), 0, data);
}

void SocketDriver::close(FileMetadata* meta) {
    if (!meta) return;
    SocketData* data = (SocketData*)meta->driver_data();
    if (data) {
        switch (data->Type) {
        case SocketType::LENSOR: {
            SocketBuffers* buffers = (SocketBuffers*)data->Data;
            buffers->RefCount -= 1;
            if (buffers->RefCount == 0)
                delete buffers;
        } break;
        }
        delete data;
    }
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
    return -1;
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
