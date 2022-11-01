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

#ifndef LENSOR_OS_VIRTUAL_FILESYSTEM_H
#define LENSOR_OS_VIRTUAL_FILESYSTEM_H

#include "smart_pointer.h"
#include "storage/device_drivers/dbgout.h"
#include "vector"

#include <file.h>
#include <filesystem.h>
#include <linked_list.h>
#include <memory>
#include <storage/file_metadata.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <string.h>
#include <scheduler.h>

struct OpenFileDescription {
    OpenFileDescription(StorageDeviceDriver* driver, const FileMetadata& md)
        : DeviceDriver(driver), Metadata(md) {}

    StorageDeviceDriver* DeviceDriver { nullptr };
    FileMetadata Metadata;
};

struct MountPoint {
    MountPoint() = default;
    MountPoint(const char* path, Filesystem* fs)
        : Path(path), FS(fs) {}

    const char* Path { nullptr };
    Filesystem* FS { nullptr };
};

enum struct ProcessFileDescriptor : FileDescriptor { Invalid = static_cast<FileDescriptor>(-1) };
enum struct GlobalFileDescriptor : FileDescriptor { Invalid = static_cast<FileDescriptor>(-1) };

struct FileDescriptors {
    ProcessFileDescriptor Process { ProcessFileDescriptor::Invalid };
    GlobalFileDescriptor Global { GlobalFileDescriptor::Invalid };

    bool valid() const {
        return Process != ProcessFileDescriptor::Invalid && Global != GlobalFileDescriptor::Invalid;
    }
};

class VFS {
    auto procfd_to_fd(ProcessFileDescriptor procfd) const -> GlobalFileDescriptor;
    void free_fd(GlobalFileDescriptor fd, ProcessFileDescriptor procfd);
    auto file(ProcessFileDescriptor fd) -> std::shared_ptr<OpenFileDescription>;
    auto file(GlobalFileDescriptor fd) -> std::shared_ptr<OpenFileDescription>;
    bool valid(ProcessFileDescriptor procfd) const;
    bool valid(GlobalFileDescriptor fd) const;
public:
    VFS() {}

    void mount(const char* path, Filesystem* fs) { Mounts.push_back(MountPoint{path, fs}); }

    FileDescriptors open(const String& path);
    FileDescriptors open(const char* path) {
        return open(String(path));
    }

    bool close(ProcessFileDescriptor procfd);

    ssz read(ProcessFileDescriptor procfd, u8* buffer, usz byteCount, usz byteOffset = 0);
    ssz write(ProcessFileDescriptor procfd, u8* buffer, usz byteCount, usz byteOffset);


    void print_debug();

    /// Set the driver that handles stdout. Returns the old driver.
    std::unique_ptr<DbgOutDriver> set_stdout_driver(std::unique_ptr<DbgOutDriver>&& driver) {
        auto old = std::move(StdoutDriver);
        StdoutDriver = std::move(driver);
        return old;
    }

    /// Files are stored as shared_ptrs to support dup() more easily.
    FileDescriptors add_file(std::shared_ptr<OpenFileDescription>, Process* proc = nullptr);

private:
    /// TODO: Should these two be protected by a lock?
    std::vector<std::shared_ptr<OpenFileDescription>> Opened;
    std::vector<size_t> FreeFileDescriptors;

    std::vector<MountPoint> Mounts;
    std::unique_ptr<DbgOutDriver> StdoutDriver;
};

#endif /* LENSOR_OS_VIRTUAL_FILESYSTEM_H */
