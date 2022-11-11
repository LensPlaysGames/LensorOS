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
#include <extensions>
#include <vector>
#include <format>
#include <storage/file_metadata.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <storage/device_drivers/dbgout.h>
#include <storage/device_drivers/pipe.h>
#include <string.h>
#include <scheduler.h>
#include <vfs_forward.h>

namespace std {
template <>
struct formatter<ProcFD> : formatter<FileDescriptor> {
    using formatter<FileDescriptor>::parse;

    template <typename FormatContext>
    auto format(const ProcFD& fd, FormatContext& ctx) {
        format_to(ctx.out(), "procfd:");
        return formatter<FileDescriptor>::format(static_cast<FileDescriptor>(fd), ctx);
    }
};

template <>
struct formatter<SysFD> : formatter<FileDescriptor> {
    using formatter<FileDescriptor>::parse;

    template <typename FormatContext>
    auto format(const SysFD& fd, FormatContext& ctx) {
        format_to(ctx.out(), "sysfd:");
        return formatter<FileDescriptor>::format(static_cast<FileDescriptor>(fd), ctx);
    }
};
} // namespace std

struct MountPoint {
    MountPoint() = default;
    MountPoint(std::string path, std::shared_ptr<FilesystemDriver>&& fs)
        : Path(std::move(path)), FS(std::move(fs)) {}

    std::string Path;
    std::shared_ptr<FilesystemDriver> FS;
};

struct FileDescriptors {
    ProcFD Process { ProcFD::Invalid };
    SysFD Global { SysFD::Invalid };

    bool valid() const {
        return Process != ProcFD::Invalid && Global != SysFD::Invalid;
    }
};

struct VFS {
    std::shared_ptr<DbgOutDriver> StdoutDriver;
    std::shared_ptr<PipeDriver> PipesDriver;

    VFS() {
        StdoutDriver = std::make_shared<DbgOutDriver>();
        PipesDriver = std::make_shared<PipeDriver>();
    }

    void mount(std::string path, std::shared_ptr<FilesystemDriver>&& fs) {
        Mounts.push_back(MountPoint{std::move(path), std::move(fs)});
    }

    const std::vector<MountPoint>& mounts() const { return Mounts; }

    /// The second file descriptor given will be associated with the file
    /// description of the first.
    bool dup2(ProcFD fd, ProcFD replaced) {
        if (!valid(fd) || !valid(replaced)) {
            return false;
        }
        SysFD sysfd = procfd_to_fd(replaced);
        if (sysfd == SysFD::Invalid) {
            return false;
        }
        auto f = file(fd);
        if (!f) {
            return false;
        }
        Files[sysfd] = std::move(f);
        return true;
    }

    FileDescriptors open(std::string_view);

    bool close(ProcFD procfd);

    ssz read(ProcFD procfd, u8* buffer, usz byteCount, usz byteOffset = 0);
    ssz write(ProcFD procfd, u8* buffer, usz byteCount, usz byteOffset);

    void print_debug();

    /// Files are stored as shared_ptrs to support dup() more easily.
    FileDescriptors add_file(std::shared_ptr<FileMetadata>, Process* proc = nullptr);

private:
    std::sparse_vector<std::shared_ptr<FileMetadata>, nullptr, SysFD> Files;
    std::vector<MountPoint> Mounts;

    auto procfd_to_fd(ProcFD procfd) const -> SysFD;
    auto file(ProcFD fd) -> std::shared_ptr<FileMetadata>;
    auto file(SysFD fd) -> std::shared_ptr<FileMetadata>;
    void free_fd(SysFD fd, ProcFD procfd);
    bool valid(ProcFD procfd) const;
    bool valid(SysFD fd) const;
};

#endif /* LENSOR_OS_VIRTUAL_FILESYSTEM_H */
