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
#include <linked_list.h>
#include <storage/file_metadata.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <storage/filesystem_drivers/dbgout.h>
#include <storage/filesystem_drivers/input.h>
#include <storage/filesystem_drivers/pipe.h>
#include <storage/filesystem_drivers/socket.h>
#include <scheduler.h>
#include <vfs_forward.h>

#include <memory>
#include <extensions>
#include <vector>
#include <format>
#include <string>

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

    bool invalid() const {
        return !valid();
    }
};

struct VFS {
    // TODO: Make all of these FilesystemDrivers rather than StorageDeviceDrivers
    std::shared_ptr<InputDriver> StdinDriver;
    std::shared_ptr<DbgOutDriver> StdoutDriver;
    std::shared_ptr<PipeDriver> PipesDriver;
    std::shared_ptr<SocketDriver> SocketsDriver;

    VFS() {
        StdinDriver    = std::make_shared<InputDriver>();
        StdoutDriver   = std::make_shared<DbgOutDriver>();
        PipesDriver    = std::make_shared<PipeDriver>();
        SocketsDriver  = std::make_shared<SocketDriver>();
    }

    void mount(std::string path, std::shared_ptr<FilesystemDriver>&& fs) {
        Mounts.push_back(MountPoint{std::move(path), std::move(fs)});
    }

    const std::vector<MountPoint>& mounts() const { return Mounts; }

    /// The returned file descriptors will be associated with the file
    /// description of the given file descriptor.
    FileDescriptors dup(Process* proc, ProcFD fd) {
        if (!proc || !valid(proc, fd)) return {};
        SysFD sysfd = procfd_to_fd(proc, fd);
        auto f = file(sysfd);
        if (!f) return {};
        return add_file(std::move(f), proc);
    }

    /// The second file descriptor given will be associated with the file
    /// description of the first.
    bool dup2(Process* proc, ProcFD fd, ProcFD replaced) {
        if (!proc) {
            std::print("[VFS]:dup2: Rejecting NULL process\n");
            return false;
        }
        if (!valid(proc, fd)) {
            std::print("[VFS]:dup2: Rejecting to replace {} with invalid process file descriptor {}\n", replaced, fd);
            return false;
        }
        if (!valid(proc, replaced)) {
            std::print("[VFS]:dup2: Rejecting to replace invalid process file descriptor {} with {}\n", replaced, fd);
            return false;
        }
        // If oldfd is a valid file descriptor, and newfd has the same
        // value as oldfd, then dup2() does nothing, and returns newfd.
        if (fd == replaced) {
            std::print("[VFS]:dup2: Process file descriptor {} is already equal to {}\n", replaced, fd);
            return true;
        }
        SysFD sysfd = procfd_to_fd(proc, fd);
        auto f = file(sysfd);
        if (!f) {
            std::print("[VFS]:dup2: ProcFD {} mapped to SysFD {} did not return a valid FileMetadata\n", fd, sysfd);
            return false;
        }

        // Get meta currently associated with `replaced` ProcFD.
        SysFD replaced_sysfd = procfd_to_fd(proc, replaced);
        //auto replaced_file = file(replaced_sysfd);
        //std::print("[VFS]:dup2: Replacing \"{}\" (ProcFD {}) with \"{}\" (ProcFD {})\n", replaced_file->name(), replaced, f->name(), fd);

        // FIXME: This assumes that each and every process file
        // descriptor refers to a unique FileMetadata in the VFS Files
        // vector. However! We should allow multiple processes to have
        // fds that refer to *one* entry in the VFS Files vector. To
        // accomplish this, we will need to maybe add an intrusive
        // refcount to filemetadata, or something similar.

        // Remove kernel file description from VFS list of open files using
        // System File Descriptor. This decrements the refcount of the
        // shared_ptr, which *may* call the destructor on the driver of
        // the actual file, if it's the last occurence.
        // FIXME: In the future, this should probably be an intrusive
        // refcount that just gets decremented, that way we don't need
        // copies of shared_ptrs in the kernel Files table.
        Files.replace(replaced_sysfd, std::move(f));
        // NOTE: Because we are replacing the file metadata in the VFS
        // Files table, this means that the process' FileDescriptor
        // still maps to the same SysFD. It's just the data stored *at*
        // that SysFD that changes. This may change in the future, and
        // the proc->FileDescriptors mappings may need altered.
        return true;
    }

    FileDescriptors open(std::string_view);

    bool close(ProcFD procfd);
    bool close(Process*, ProcFD procfd);

    ssz read(ProcFD procfd, u8* buffer, usz byteCount, usz byteOffset = 0);
    ssz write(ProcFD procfd, u8* buffer, usz byteCount, usz byteOffset);

    void print_debug();

    FileDescriptors add_file(std::shared_ptr<FileMetadata>, Process* proc = nullptr);

    auto procfd_to_fd(ProcFD procfd) const -> SysFD;
    auto procfd_to_fd(Process*, ProcFD procfd) const -> SysFD;
    auto file(ProcFD fd) -> std::shared_ptr<FileMetadata>;
    auto file(SysFD fd) -> std::shared_ptr<FileMetadata>;

    ssz directory_data(std::string_view path, usz entry_count, DirectoryEntry* dirents) {
        if (not entry_count) return 0;

        if (not path.size() or path == std::string_view("/")) {
            // Fill dirents with MountPoint prefixes (iterate Mounts)
            usz count = 0;
            for (auto mount : Mounts) {
                // Copy mount prefix into directory entry name field.
                memcpy(&dirents[count].name[0], mount.Path.data(), mount.Path.size());
                // Root of mount is always a directory
                dirents[count].type = FileMetadata::FileType::Directory;
                ++count;
            }
            return count;
        }

        // If path doesn't point to root, validate it somewhat.
        if (not path.starts_with("/")) return -1;

        // Get FilesystemDriver from MountPoint by matching prefix, then hand it
        // over to the FSD.
        for (auto mount : Mounts) {
            /// It makes no sense to search file systems whose mount point does not
            /// match the beginning of the path. And even if they’re mounted twice,
            /// we’ll still find the second mount.
            if (!path.starts_with(mount.Path)) continue;
            auto fs_path = path.substr(mount.Path.size());
            return mount.FS->directory_data(fs_path, entry_count, dirents);
        }
        return -1;
    }

private:
    std::sparse_vector<std::shared_ptr<FileMetadata>, nullptr, SysFD> Files;
    std::vector<MountPoint> Mounts;

    void free_fd(SysFD fd, ProcFD procfd);
    void free_fd(Process*, SysFD fd, ProcFD procfd);
    bool valid(Process *proc, ProcFD procfd) const;
    bool valid(ProcFD procfd) const;
    bool valid(SysFD fd) const;
};

#endif /* LENSOR_OS_VIRTUAL_FILESYSTEM_H */
