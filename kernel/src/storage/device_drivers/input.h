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

#ifndef LENSOR_OS_INPUT_DRIVER_H
#define LENSOR_OS_INPUT_DRIVER_H

// Uncomment the following directive for extra debug information output.
//#define DEBUG_INPUT_DRIVER


#include <format>
#include <string>
#include <string_view>
#include <vector>

#include <memory/common.h>
#include <storage/storage_device_driver.h>
#include <storage/file_metadata.h>

#ifdef DEBUG_INPUT_DRIVER
# define DBGMSG(...) std::print(__VA_ARGS__)
#else
# define DBGMSG(...)
#endif

// NOTE: This is an attempt to keep `sizeof(InputBuffer)` == PAGE_SIZE
#define INPUT_BUFSZ PAGE_SIZE - sizeof(usz)

struct InputBuffer {
    u8 Data[INPUT_BUFSZ];
    usz Offset{};

    constexpr InputBuffer() = default;
    ~InputBuffer() = default;

    /// Copying an input buffer is nonsense.
    InputBuffer(const InputBuffer&) = delete;

    /// We don’t allow moving either so we don’t accidentally move an
    /// input buffer while someone is reading from or writing to it.
    InputBuffer(InputBuffer&&) = delete;
};

struct NamedInputBuffer {
    std::weak_ptr<FileMetadata> Meta;
    std::string Name;
    InputBuffer* Buffer;
};

struct InputDriver final : StorageDeviceDriver {
    void close(FileMetadata* file) final {
        if (!file) return;
        auto* input = static_cast<InputBuffer*>(file->driver_data());
        FreeInputBuffers.push_back(input);
    }

    std::shared_ptr<FileMetadata> open(std::string_view path) final;

    ssz read_raw(usz, usz, void*) final {
        return -1;
    }

    ssz read(FileMetadata* file, usz, usz bytes, void* buffer) final {
        if (!file) return -1;

        auto* input = static_cast<InputBuffer*>(file->driver_data());
        if (!input) return -1;

        // TODO: Support "wait until there is something to read".
        if (input->Offset == 0) {
            DBGMSG("[INPUT]: Input buffer at {} has no data (todo: wait)\n", (void*)input);
            return -1;
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

    ssz write(FileMetadata* file, usz, usz bytes, void* buffer) final {
        if (!file) return -1;
        auto* input = static_cast<InputBuffer*>(file->driver_data());
        if (!input) return -1;
        if (input->Offset + bytes > INPUT_BUFSZ) {
            // TODO: Support "wait if full". For now, just truncate write.
            bytes = INPUT_BUFSZ - input->Offset;
        }
        memcpy(input->Data + input->Offset, buffer, bytes);
        input->Offset += bytes;
        return ssz(bytes);
    }

private:
    std::vector<NamedInputBuffer> InputBuffers;
    std::vector<InputBuffer*> FreeInputBuffers;
};

#undef DBGMSG

#endif /* LENSOR_OS_INPUT_DRIVER_H */
