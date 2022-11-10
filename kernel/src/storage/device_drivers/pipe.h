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

#include <debug.h>
#include <integers.h>
#include <storage/storage_device_driver.h>

#define PIPE_BUFSZ 512

struct PipeBuffer {
    u8* Data;
    usz Offset = 0;

    PipeBuffer() {
        Data = new u8[PIPE_BUFSZ];
    }

    PipeBuffer(const PipeBuffer&) = delete;

    ~PipeBuffer() {
        delete[] Data;
    }
};

class PipeDriver final : public StorageDeviceDriver {
public:
    ssz read(usz byteOffset, usz byteCount, void* buffer) final {
        // Find which pipe by using byte offset.
        if (byteOffset >= PipeBuffers.size()) {
            return -1;
        }
        PipeBuffer& pipeBuffer = PipeBuffers[byteOffset];

        // TODO: Support "wait until there is something to read".
        if (pipeBuffer.Offset == 0) {
            return -1;
        }

        // TODO: Read in a loop to fill buffers larger than what is currently written.
        if (byteCount > pipeBuffer.Offset) {
            byteCount = pipeBuffer.Offset;
        }

        memcpy(buffer, pipeBuffer.Data + pipeBuffer.Offset, byteCount);
        return byteCount;
    };
    ssz write(usz byteOffset, usz byteCount, void* buffer) final {
        // Find which pipe by using byte offset. There is no filesystem
        // backing, so we can just use that as an index/key to find
        // which buffer to write to.
        if (byteOffset >= PipeBuffers.size()) {
            return -1;
        }
        PipeBuffer& pipeBuffer = PipeBuffers[byteOffset];
        if (pipeBuffer.Offset + byteCount > PIPE_BUFSZ) {
            // TODO: Support "wait if full". For now, just truncate write.
            byteCount = PIPE_BUFSZ - pipeBuffer.Offset;
        }

        memcpy(pipeBuffer.Data + pipeBuffer.Offset, buffer, byteCount);
        return byteCount;
    }

    /// Return a byte offset that can be used later to find a unique
    /// buffer, allocated by this function.
    ssz lay_pipe() {
        PipeBuffers.push_back({});
        return PipeBuffers.size() - 1;
    }

private:
    std::vector<PipeBuffer> PipeBuffers;
};

#endif /* LENSOR_OS_PIPE_DRIVER_H */
