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
#   define DBGMSG(...) dbgmsg(__VA_ARGS__)
#else
#   define DBGMSG(...)
#endif

SysFD VFS::procfd_to_fd(ProcFD procfd) const {
    const auto& proc = Scheduler::CurrentProcess->value();
    auto sysfd = proc->FileDescriptors[procfd];
    if (!sysfd) {
        DBGMSG("[VFS]: ERROR: ProcFD %x:%ull is unmapped.\r\n", &proc, u64(procfd));
        return SysFD::Invalid;
    }

    return *sysfd;
}

auto VFS::file(ProcFD procfd) -> std::shared_ptr<OpenFileDescription> {
    const auto& proc = Scheduler::CurrentProcess->value();

#ifdef DEBUG_VFS
    dbgmsg("[VFS]: ProcFds for process %x:\r\n", &proc);
    u64 n = 0;
    for (const auto& entry : proc->FileDescriptors) {
        dbgmsg("  %ull -> Sys %ull\r\n", n, entry);
        n++;
    }
#endif

    auto sysfd = proc->FileDescriptors[procfd];
    if (!sysfd) {
        DBGMSG("[VFS]: ERROR: ProcFD %x:%ull is unmapped.\r\n", &proc, u64(procfd));
        return {};
    }

    DBGMSG("[VFS]: file: ProcFD %x:%ull is mapped to SysFD %ull.\r\n", &proc, u64(procfd), *sysfd);
    return file(static_cast<SysFD>(*sysfd));
}

auto VFS::file(SysFD fd) -> std::shared_ptr<OpenFileDescription> {
    auto f = Files[fd];
    if (!f) {
        DBGMSG("[VFS]: ERROR: SysFD %ull is unmapped.\r\n", fd);
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
        DBGMSG("[VFS]: ERROR: SysFD %ull is unmapped.\r\n", u64(fd));
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
        dbgmsg_s("[VFS]: path is not long enough.\r\n");
        return {};
    }
    if (path[0] != '/') {
        dbgmsg("[VFS]: path does not start with slash, %s\r\n", fullPathLength);
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
                    DBGMSG("  Metadata:\r\n"
                           "    Name: %sl\r\n"
                           "    File Size: %ull\r\n"
                           "    Byte Offset: %ull\r\n"
                           "    Filesystem Driver: %x\r\n"
                           "    Device Driver: %x\r\n"
                           "    Invalid: %b\r\n"
                           , metadata.name()
                           , metadata.file_size()
                           , metadata.byte_offset()
                           , metadata.file_driver()
                           , metadata.device_driver()
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
        DBGMSG("[VFS]: Cannot close invalid ProcFD %x:%ull.\r\n", &proc, u64(procfd));
        return false;
    }

    auto f = file(fd);
    if (!f) {
        DBGMSG("[VFS]: Cannot close invalid SysFD %ull.\r\n", u64(fd));
        return false;
    }

    DBGMSG("[VFS]: Unmapping ProcFD %x:%ull.\r\n", &proc, u64(procfd));
    DBGMSG("[VFS]: Closing SysFD %ull.\r\n", u64(fd));
    free_fd(fd, procfd);
    return true;
}

ssz VFS::read(ProcFD fd, u8* buffer, usz byteCount, usz byteOffset) {
#ifdef DEBUG_VFS
    dbgmsg("[VFS]: read\r\n"
           "  file descriptor: %ull\r\n"
           "  buffer address:  %x\r\n"
           "  byte count:      %ull\r\n"
           "  byte offset:     %ull\r\n"
           , fd
           , buffer
           , byteCount
           , byteOffset
           );
#endif /* #ifdef DEBUG_VFS */

    auto f = file(fd);
    if (!f) { return -1; }

    return f->DeviceDriver->read(f->Metadata.byte_offset() + byteOffset, byteCount, buffer);
}

ssz VFS::write(ProcFD fd, u8* buffer, u64 byteCount, u64 byteOffset) {
    auto f = file(fd);

    DBGMSG("[VFS]: write\r\n"
           "  file descriptor: %ull\r\n"
           "  buffer address:  %x\r\n"
           "  byte count:      %ull\r\n"
           "  byte offset:     %ull\r\n"
           , fd
           , buffer
           , byteCount
           , byteOffset
           );

    return f->DeviceDriver->write(f->Metadata.byte_offset() + byteOffset, byteCount, buffer);
}

void VFS::print_debug() {
    dbgmsg("[VFS]: Debug Info\r\n"
           "  Mounts:\r\n");
    u64 i = 0;
    for (const auto& mp : Mounts) {
        dbgmsg("    Mount %ull:\r\n"
               "      Path: %s\r\n"
               "      Filesystem: %s\r\n"
               "        Filesystem Driver Address: %x\r\n"
               "        Storage Device Driver Address: %x\r\n"
               , i
               , mp.Path
               , Filesystem::type2name(mp.FS->type())
               , mp.FS->filesystem_driver()
               , mp.FS->storage_device_driver()
               );
        i += 1;
    }
    dbgmsg("\r\n"
           "  Opened files:\r\n");
    i = 0;
    for (const auto& f : Files) {
        dbgmsg("    Open File %ull:\r\n"
               "      Storage Device Driver Address: %x\r\n"
               "      Byte Offset: %ull\r\n"
               , i
               , f->DeviceDriver
               , f->Metadata.byte_offset()
        );
        i++;
    }
    dbgmsg("\r\n");
}

FileDescriptors VFS::add_file(std::shared_ptr<OpenFileDescription> file, Process* proc) {
    if (!proc) proc = Scheduler::CurrentProcess->value();
    DBGMSG("[VFS]: Creating file descriptor mapping\r\n");

    /// Add the file descriptor to the global file table.
    auto [fd, _] = Files.push_back(std::move(file));
    DBGMSG("[VFS]: Allocated new SysFD %ull\r\n", fd);

    /// Add the file descriptor to the local process table.
    auto [procfd, __] = proc->FileDescriptors.push_back(fd);
    DBGMSG("[VFS]: Allocated new ProcFD %x:%ull\r\n", &proc, procfd);

    /// Return the fds.
    DBGMSG("[VFS]: Mapped ProcFD %x:%ull to SysFD %ull\r\n", &proc, procfd, fd);
    return {static_cast<ProcFD>(procfd), static_cast<SysFD>(fd)};
}
