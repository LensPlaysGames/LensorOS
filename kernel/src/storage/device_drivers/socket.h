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

template <usz N = KiB(1)>
struct FIFOBuffer {
    u8 Data[N] {0};
    usz Size {N};
    /// This index is the index that incoming data will be written to.
    usz Offset {0};
    /// List of PIDs of processes who are waiting to write into the
    /// txbuffer as it is full.
    std::vector<pid_t> PIDsWaitingUntilRead;
    /// List of PIDs of processes who are waiting to read from the
    /// txbuffer as it is empty.
    std::vector<pid_t> PIDsWaitingUntilWrite;

    void clear() {
        memset(&Data[0], 0, sizeof(Data));
        Offset = 0;
        // TODO: We may want to run any processes in these lists.
        PIDsWaitingUntilRead.clear();
        PIDsWaitingUntilWrite.clear();
    }

    /// Read `byteCount` bytes from this FIFOBuffer, writing them into `buffer`.
    /// \param pid
    ///   The ID of the process that is performing the read.
    /// \retval >=0  Success, amount of bytes read.
    /// \retval -1   Failure
    /// \retval -2   Should block (will unblock when written to)
    ssz read(pid_t pid, usz byteCount, u8* buffer) {
        if (Offset == 0) {
            PIDsWaitingUntilWrite.push_back(pid);
            return -2;
        }

        // Truncate reads that are larger than possible.
        if (byteCount > Offset)
            byteCount = Offset;

        // Transfer the data from the FIFO buffer to the given buffer.
        memcpy(buffer, Data, byteCount);

        // "Pop" data off the front of the FIFO buffer.
        memmove(&Data[0], &Data[byteCount], Size - byteCount);
        Offset -= byteCount;

        return byteCount;
    }

    /// Write `byteCount` bytes to this FIFOBuffer from `buffer`.
    /// \param pid
    ///   The ID of the process that is performing the write.
    /// \retval >=0  Success, amount of bytes written.
    /// \retval -1   Failure
    /// \retval -2   Should block (will unblock when read from)
    ssz write(pid_t pid, usz byteCount, u8* buffer) {
        if (Offset == 0 || Offset + byteCount > Size) {
            PIDsWaitingUntilRead.push_back(pid);
            return -2;
        }

        // Transfer the data from the given buffer to the FIFO buffer.
        memcpy(Data + Offset, buffer, byteCount);

        // Move write offset for next time.
        Offset += byteCount;

        return byteCount;
    }
};

#define SOCKET_TX_BUFFER_SIZE (PAGE_SIZE / 2)
#define SOCKET_RX_BUFFER_SIZE ((PAGE_SIZE / 2) - sizeof(usz))
struct SocketBuffers {
    /// FIFO buffer for transmissions from the server.
    /// Server writes to this buffer.
    /// Client reads from this buffer.
    FIFOBuffer<SOCKET_TX_BUFFER_SIZE> TXBuffer;
    /// FIFO buffer for receiving data from a client.
    /// Server reads from this buffer.
    /// Client writes to this buffer.
    FIFOBuffer<SOCKET_RX_BUFFER_SIZE> RXBuffer;

    usz RefCount {1};

    void clear() {
        TXBuffer.clear();
        RXBuffer.clear();
    }
};

enum class SocketType {
    /// A Lensor socket is a socket intended for interprocess
    /// communication on the local machine.
    LENSOR,
};

#define SOCKET_ADDRESS_MAX_SIZE 16
struct SocketAddress {
    enum {
        // 16 bytes; simply a unique identifier which is memcmp'd
        // Used by LENSOR type sockets.
        LENSOR16,
    } Type;
    u8 Data[SOCKET_ADDRESS_MAX_SIZE];
};

/// Each FileMetadata associated with an open socket has this struct at
/// it's `driver_data()`.
struct SocketData {
    SocketType Type;
    SocketAddress Address;
    enum {
        CLIENT,
        SERVER
    } ClientServer {CLIENT};
    // We *could* make this a base class and have each socket type
    // implement it's own read, write, etc but I think the `void*` is
    // fine for now.
    void* Data;
};

struct SocketDriver final : StorageDeviceDriver {
    void close(FileMetadata* meta) final;
    auto open(std::string_view path) -> std::shared_ptr<FileMetadata> final;

    auto socket(SocketType domain, int type, int protocol) -> std::shared_ptr<FileMetadata>;

    ssz read(FileMetadata* meta, usz, usz byteCount, void* buffer) final;
    ssz read_raw(usz, usz, void*) final { return -1; };
    ssz write(FileMetadata* meta, usz, usz byteCount, void* buffer) final;
};

#endif /* LENSOR_OS_SOCKET_DRIVER_H */
