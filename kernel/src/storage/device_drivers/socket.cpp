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

#include <storage/file_metadata.h>
#include <storage/storage_device_driver.h>
#include <storage/device_drivers/socket.h>
#include <system.h>

#include <memory>

std::shared_ptr<FileMetadata> SocketDriver::open(std::string_view path) {
    SocketData* data = new SocketData;
    return std::make_shared<FileMetadata>("new_socket", sdd(SYSTEM->virtual_filesystem().SocketsDriver), 0, data);
}

void SocketDriver::close(FileMetadata* meta) {
    if (meta->driver_data()) {
        SocketData* data = (SocketData*)meta->driver_data();
        switch (data->Type) {
        case SocketType::LENSOR:
            delete (SocketBuffers*)data->Data;
            break;
        }
        delete data;
    }
}

ssz SocketDriver::read(FileMetadata* meta, usz, usz byteCount, void* buffer) {
    SocketData* data = (SocketData*)meta->driver_data();
    switch (data->Type) {
    case SocketType::LENSOR:
        switch (data->ClientServer) {
        case SocketData::CLIENT:
            // TODO: Read from TXBuffer.
            break;
        case SocketData::SERVER:
            // TODO: Read from RXBuffer.
            break;
        }
        break;
    }
    std::print("[SOCK]:TODO: Implement `SocketDriver::read()`");
    return -1;
}

ssz SocketDriver::write(FileMetadata* meta, usz, usz byteCount, void* buffer) {
    SocketData* data = (SocketData*)meta->driver_data();
    switch (data->Type) {
    case SocketType::LENSOR:
        switch (data->ClientServer) {
        case SocketData::CLIENT:
            // TODO: write to RXBuffer.
            break;
        case SocketData::SERVER:
            // TODO: Write to TXBuffer.
            break;
        }
        break;
    }
    std::print("[SOCK]:TODO: Implement `SocketDriver::write()`");
    return -1;
}
