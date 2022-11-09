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


#include <file.h>
#include <filesystem.h>
#include <linked_list.h>
#include <memory>
#include <smart_pointer.h>
#include <extensions>
#include <vector>
#include <storage/file_metadata.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <storage/device_drivers/dbgout.h>
#include <storage/device_drivers/pipe.h>
#include <string.h>
#include <scheduler.h>
#include <vfs_forward.h>

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

struct FileDescriptors {
    ProcFD Process { ProcFD::Invalid };
    SysFD Global { SysFD::Invalid };

    bool valid() const {
        return Process != ProcFD::Invalid && Global != SysFD::Invalid;
    }
};

class VFS {
    auto procfd_to_fd(ProcFD procfd) const -> SysFD;
    void free_fd(SysFD fd, ProcFD procfd);
    auto file(ProcFD fd) -> std::shared_ptr<OpenFileDescription>;
    auto file(SysFD fd) -> std::shared_ptr<OpenFileDescription>;
    bool valid(ProcFD procfd) const;
    bool valid(SysFD fd) const;
public:
    std::unique_ptr<DbgOutDriver> StdoutDriver;
    std::unique_ptr<PipeDriver> PipesDriver;

    VFS() {}

    void mount(const char* path, Filesystem* fs) { Mounts.push_back(MountPoint{path, fs}); }

    FileDescriptors open(const String& path);
    FileDescriptors open(const char* path) {
        return open(String(path));
    }

    bool close(ProcFD procfd);

    ssz read(ProcFD procfd, u8* buffer, usz byteCount, usz byteOffset = 0);
    ssz write(ProcFD procfd, u8* buffer, usz byteCount, usz byteOffset);

    void print_debug();

    /// Files are stored as shared_ptrs to support dup() more easily.
    FileDescriptors add_file(std::shared_ptr<OpenFileDescription>, Process* proc = nullptr);

private:
    std::sparse_vector<std::shared_ptr<OpenFileDescription>, nullptr, SysFD> Files;
    std::vector<MountPoint> Mounts;
};

#endif /* LENSOR_OS_VIRTUAL_FILESYSTEM_H */
