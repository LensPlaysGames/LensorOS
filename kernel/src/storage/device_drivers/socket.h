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

#ifndef LENSOR_OS_SOCKET_DRIVER_H
#define LENSOR_OS_SOCKET_DRIVER_H

#include <integers.h>
#include <scheduler.h>

#include <vector>

/// A socket FileMetadata, when opened, is basically just an empty
/// metadata (name only). The real filling in of the metadata happens
/// on a call to `bind` or `connect` for a server or client, respectively.
/// Note that both `bind` and `connect` will treat the given address
/// differently depending on the type of the opened socket.

/// `bind` is called by a server to associate a socket with an address.
/// Basically, it allows the server to choose what address it is bound
/// to. The address that clients will need to use to connect to the
/// server.
/// In the case of a Lensor socket, this is simply a 16-byte piece of
/// memory that uniquely identifies it. Most of the time, this will be
/// a string with a somewhat descriptive name, i.e. "/WindowServer   "
/// or similar.

/// `listen` basically just signifies that the socket is willing to
/// `accept` requests as they come in. It is used to specify the port.

/// `accept` may be called after a server calls `bind` and `listen`.
/// The process is blocked until a connection request is received (some
/// client called `connect`), at which point it will return a new file
/// descriptor referring to a new socket. *This* returned socket is
/// used to communicate with the client that called `connect`.

/// `connect` is called by a client to attempt to make a connection to
/// the socket bound to the address specified by the one given.
/// If the connection or binding succeeds, zero is returned. Any other
/// return value means the socket should be closed. To attempt a retry,
/// open a new socket.

#define SOCKET_TX_BUFFER_SIZE 1024
#define SOCKET_RX_BUFFER_SIZE 1024
struct SocketBuffers {
    /// FIFO buffer for transmissions from the server.
    /// Server writes to this buffer.
    /// Client reads from this buffer.
    u8 TXBuffer[SOCKET_TX_BUFFER_SIZE] {0};
    /// This index is the index that incoming data will be written to.
    usz TXOffset {0};
    /// List of PIDs of processes who are waiting to write into the
    /// txbuffer as it is full.
    std::vector<pid_t> PIDsWaitingTXFull;
    /// List of PIDs of processes who are waiting to read from the
    /// txbuffer as it is empty.
    std::vector<pid_t> PIDsWaitingTXEmpty;

    /// FIFO buffer for receiving data from a client.
    /// Server reads from this buffer.
    /// Client writes to this buffer.
    u8 RXBuffer[SOCKET_RX_BUFFER_SIZE] {0};
    usz RXOffset {0};
    std::vector<pid_t> PIDsWaitingRXFull;
    std::vector<pid_t> PIDsWaitingRXEmpty;

    void clear() {
        memset(&TXBuffer[0], 0, sizeof(TXBuffer));
        TXOffset = 0;
        PIDsWaitingTXFull.clear();
        PIDsWaitingTXEmpty.clear();

        memset(&RXBuffer[0], 0, sizeof(RXBuffer));
        RXOffset = 0;
        PIDsWaitingRXFull.clear();
        PIDsWaitingRXEmpty.clear();
    }
};

enum class SocketType {
    /// A Lensor socket is a socket intended for interprocess
    /// communication on the local machine.
    LENSOR,
};

struct SocketData {
    SocketType Type;
    // We *could* make this a base class and have each socket type
    // implement it's own read, write, etc but I think the `void*` is
    // fine for now.
    void* Data;
};

struct SocketDriver final : StorageDeviceDriver {
    void close(FileMetadata* meta) final;
    auto open(std::string_view path) -> std::shared_ptr<FileMetadata> final;

    ssz read(FileMetadata* meta, usz, usz byteCount, void* buffer) final;
    ssz read_raw(usz, usz, void*) final { return -1; };
    ssz write(FileMetadata* meta, usz, usz byteCount, void* buffer) final;
};

#endif /* LENSOR_OS_SOCKET_DRIVER_H */
