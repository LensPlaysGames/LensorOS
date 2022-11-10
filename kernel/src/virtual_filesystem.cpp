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

#include <format>

#include "scheduler.h"

#include <cstr.h>
#include <debug.h>
#include <file.h>
#include <filesystem.h>
#include <linked_list.h>
#include <storage/file_metadata.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <string.h>
#include <virtual_filesystem.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_VFS

#ifdef DEBUG_VFS
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...)
#endif

SysFD VFS::procfd_to_fd(ProcFD procfd) const {
    const auto& proc = Scheduler::CurrentProcess->value();
    auto sysfd = proc->FileDescriptors[procfd];
    if (!sysfd) {
        DBGMSG("[VFS]: ERROR {} (pid {}) is unmapped.\n", procfd, proc->ProcessID);
        return SysFD::Invalid;
    }

    return *sysfd;
}

auto VFS::file(ProcFD procfd) -> std::shared_ptr<OpenFileDescription> {
    const auto& proc = Scheduler::CurrentProcess->value();

#ifdef DEBUG_VFS
    std::print("[VFS]: ProcFds for process {}:\n", proc->ProcessID);
    u64 n = 0;
    for (const auto& entry : proc->FileDescriptors) {
        std::print("  {} -> {}\n", n, entry);
        n++;
    }
#endif

    auto sysfd = proc->FileDescriptors[procfd];
    if (!sysfd) {
        DBGMSG("[VFS]: ERROR: {} (pid {}) is unmapped.\n", procfd, proc->ProcessID);
        return {};
    }

    DBGMSG("[VFS]: file: {} ({}) is mapped to SysFD {}.\n", procfd, proc->ProcessID, *sysfd);
    return file(static_cast<SysFD>(*sysfd));
}

auto VFS::file(SysFD fd) -> std::shared_ptr<OpenFileDescription> {
    auto f = Files[fd];
    if (!f) {
        DBGMSG("[VFS]: ERROR: {} is unmapped.\n", fd);
        return {};
    }
    return *f;
}

bool VFS::valid(ProcFD procfd) const {
    return procfd_to_fd(procfd) != SysFD::Invalid;
}

bool VFS::valid(SysFD fd) const {
    auto f = Files[fd];
    if (!f) {
        DBGMSG("[VFS]: ERROR: {} is unmapped.\n", fd);
        return false;
    }
    return true;
}

void VFS::free_fd(SysFD fd, ProcFD procfd) {
    const auto& proc = Scheduler::CurrentProcess->value();
    proc->FileDescriptors.erase(procfd);
    Files.erase(fd);
}

FileDescriptors VFS::open(const String& path) {
    u64 fullPathLength = path.length();
    if (fullPathLength <= 1) {
        std::print("[VFS]: path is not long enough.\n");
        return {};
    }
    if (path[0] != '/') {
        std::print("[VFS]: path does not start with slash, {}\n", fullPathLength);
        return {};
    }

    for (const auto& mount : Mounts) {
        StorageDeviceDriver* dev = mount.FS->storage_device_driver();
        FilesystemDriver* fileDriver = mount.FS->filesystem_driver();
        u64 mountPathLength = strlen(mount.Path) - 1;
        if (mountPathLength <= fullPathLength) {
            if (strcmp(path.data(), mount.Path, mountPathLength)) {
                String prefixlessPath = path;
                prefixlessPath.chop(mountPathLength, String::Side::Right);
                if (prefixlessPath == path) {
                    // TODO: path matches a mount path exactly.
                    // How do we open a mount? Should we? NO!!
                }
                else {
                    FileMetadata metadata = fileDriver->file(dev, prefixlessPath.data());
                    DBGMSG("  Metadata:\n"
                           "    Name: {}\n"
                           "    File Size: {}\n"
                           "    Byte Offset: {}\n"
                           "    Filesystem Driver: {}\n"
                           "    Device Driver: {}\n"
                           "    Invalid: {}\n"
                           , std::string_view { metadata.name().data(), metadata.name().length() }
                           , metadata.file_size()
                           , metadata.byte_offset()
                           , (void*) metadata.file_driver()
                           , (void*) metadata.device_driver()
                           , metadata.invalid()
                           );
                    if (!metadata.invalid()) {
                        return add_file(std::make_shared<OpenFileDescription>(dev, metadata));
                    }
                }
            }
        }
    }

    return {};
}

bool VFS::close(ProcFD procfd) {
    auto fd = procfd_to_fd(procfd);
    [[maybe_unused]] auto& proc = Scheduler::CurrentProcess->value();
    if (fd == SysFD::Invalid) {
        DBGMSG("[VFS]: Cannot close invalid {} (pid {}).\n", procfd, proc->ProcessID);
        return false;
    }

    auto f = file(fd);
    if (!f) {
        DBGMSG("[VFS]: Cannot close invalid {}.\n", fd);
        return false;
    }

    DBGMSG("[VFS]: Unmapping {} (pid {}).\n", procfd, proc->ProcessID);
    DBGMSG("[VFS]: Closing {}.\n", fd);
    free_fd(fd, procfd);
    return true;
}

ssz VFS::read(ProcFD fd, u8* buffer, usz byteCount, usz byteOffset) {
    DBGMSG("[VFS]: read\n"
           "  file descriptor: {}\n"
           "  buffer address:  {}\n"
           "  byte count:      {}\n"
           "  byte offset:     {}\n"
           , fd
           , (void*) buffer
           , byteCount
           , byteOffset
           );

    auto f = file(fd);
    if (!f) { return -1; }

    return f->DeviceDriver->read(f->Metadata.byte_offset() + byteOffset, byteCount, buffer);
}

ssz VFS::write(ProcFD fd, u8* buffer, u64 byteCount, u64 byteOffset) {
    auto f = file(fd);

    DBGMSG("[VFS]: write\n"
           "  file descriptor: {}\n"
           "  buffer address:  {}\n"
           "  byte count:      {}\n"
           "  byte offset:     {}\n"
           , fd
           , (void*) buffer
           , byteCount
           , byteOffset
           );

    return f->DeviceDriver->write(f->Metadata.byte_offset() + byteOffset, byteCount, buffer);
}

void VFS::print_debug() {
    std::print("[VFS]: Debug Info\n"
           "  Mounts:\n");
    u64 i = 0;
    for (const auto& mp : Mounts) {
        std::print("    Mount {}:\n"
                   "      Path: {}\n"
                   "      Filesystem: {}\n"
                   "        Filesystem Driver Address: {}\n"
                   "        Storage Device Driver Address: {}\n"
                   , i
                   , mp.Path
                   , Filesystem::type2name(mp.FS->type())
                   , (void*) mp.FS->filesystem_driver()
                   , (void*) mp.FS->storage_device_driver()
                   );
        i += 1;
    }
    std::print("\n  Opened files:\n");
    i = 0;
    for (const auto& f : Files) {
        std::print("    Open File {}:\n"
                   "      Storage Device Driver Address: {}\n"
                   "      Byte Offset: {}\n"
                   , i
                   , (void*) f->DeviceDriver
                   , f->Metadata.byte_offset()
        );
        i++;
    }
    std::print("\n");
}

FileDescriptors VFS::add_file(std::shared_ptr<OpenFileDescription> file, Process* proc) {
    if (!proc) proc = Scheduler::CurrentProcess->value();
    DBGMSG("[VFS]: Creating file descriptor mapping\n");

    /// Add the file descriptor to the global file table.
    auto [fd, _] = Files.push_back(std::move(file));
    DBGMSG("[VFS]: Allocated new {}\n", fd);

    /// Add the file descriptor to the local process table.
    auto [procfd, __] = proc->FileDescriptors.push_back(fd);
    DBGMSG("[VFS]: Allocated new {} (pid {})\n", procfd, proc->ProcessID);

    /// Return the fds.
    DBGMSG("[VFS]: Mapped {} (pid {}) to {}\n", procfd, proc->ProcessID, fd);
    return {static_cast<ProcFD>(procfd), static_cast<SysFD>(fd)};
}
