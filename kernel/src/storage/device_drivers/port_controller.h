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
#ifndef LENSOROS_PORT_CONTROLLER_H
#define LENSOROS_PORT_CONTROLLER_H

#include <ahci.h>

namespace AHCI {

struct PortController final : StorageDeviceDriver {
    PortController(PortType type, u64 portNumber, HBAPort* portAddress);

    /// Not valid for this driver, but required by the interface.
    void close(FileMetadata*) final {}
    auto open(std::string_view) -> std::shared_ptr<FileMetadata> final
    { return std::shared_ptr<FileMetadata>{nullptr}; }

    /// Convert bytes to sectors, then read into and copy from intermediate
    /// `Buffer` to given `buffer` until all data is read and copied.
    ssz read(FileMetadata*,usz byteOffset, usz byteCount, void* buffer) final;
    ssz write(FileMetadata*,usz byteOffset, usz byteCount, void* buffer) final;

    // FIXME: I think there are a max of 32 ports, no? We can
    // probably use something smaller than a u64 here.
    u64 port_number() { return PortNumber; }

private:
    PortType Type { PortType::None };
    u64 PortNumber { 99 };
    volatile HBAPort* Port { nullptr };
    u8* Buffer { nullptr };
    const u64 BYTES_PER_SECTOR = 512;
    const u64 PORT_BUFFER_PAGES = 0x100;
    const u64 PORT_BUFFER_BYTES = PORT_BUFFER_PAGES * 0x1000;

    /// Populate `Buffer` with `sectors` amount of data starting at `sector`.
    bool read_low_level(u64 sector, u64 sectors);

    void start_commands();
    void stop_commands();
};

}

#endif // LENSOROS_PORT_CONTROLLER_H
