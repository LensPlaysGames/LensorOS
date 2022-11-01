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
#define DEBUG_VFS

GlobalFileDescriptor VFS::procfd_to_fd(ProcessFileDescriptor procfd) const {
    auto raw = static_cast<FileDescriptor>(procfd);
    const auto& proc = Scheduler::CurrentProcess->value();

    if (raw > proc->FileDescriptorTable.size() || proc->FileDescriptorTable[raw] == -1lu) {
#ifdef DEBUG_VFS
        dbgmsg("ERROR: ProcFD %ull is unmapped.\r\n", raw);
#endif
        return GlobalFileDescriptor::Invalid;
    }

#ifdef DEBUG_VFS
    dbgmsg("procfd_to_fd: ProcFD %ull is mapped to SysFD %ull.\r\n", raw, proc->FileDescriptorTable[raw]);
#endif
    return static_cast<GlobalFileDescriptor>(proc->FileDescriptorTable[raw]);
}

auto VFS::file(ProcessFileDescriptor procfd) -> std::shared_ptr<OpenFileDescription> {
    auto raw = static_cast<FileDescriptor>(procfd);
    const auto& proc = Scheduler::CurrentProcess->value();

#ifdef DEBUG_VFS
    dbgmsg("[VFS] ProcFds:\r\n");
    u64 n = 0;
    for (const auto& entry : proc->FileDescriptorTable) {
        dbgmsg("  %ull -> Sys %ull\r\n", n, entry);
        n++;
    }
#endif

    if (raw > proc->FileDescriptorTable.size() || proc->FileDescriptorTable[raw] == -1lu) {
#ifdef DEBUG_VFS
        dbgmsg("ERROR: ProcFD %ull is unmapped. (FDTable size: %ull)\r\n", raw, proc->FileDescriptorTable.size());
#endif
        return {};
    }

#ifdef DEBUG_VFS
    dbgmsg("file: ProcFD %ull is mapped to SysFD %ull.\r\n", raw, proc->FileDescriptorTable[raw]);
#endif
    return file(static_cast<GlobalFileDescriptor>(proc->FileDescriptorTable[raw]));
}

auto VFS::file(GlobalFileDescriptor fd) -> std::shared_ptr<OpenFileDescription> {
    auto raw = static_cast<FileDescriptor>(fd);
    if (raw > Opened.size() || !Opened[raw]) {
#ifdef DEBUG_VFS
        dbgmsg("ERROR: SysFD %ull is unmapped.\r\n", raw);
#endif
        return {};
    }
    return Opened[raw];
}

bool VFS::valid(ProcessFileDescriptor procfd) const {
    return procfd_to_fd(procfd) != GlobalFileDescriptor::Invalid;
}

bool VFS::valid(GlobalFileDescriptor fd) const {
    auto raw = static_cast<FileDescriptor>(fd);
    if (raw > Opened.size() || !Opened[raw]) {
#ifdef DEBUG_VFS
        dbgmsg("ERROR: SysFD %ull is unmapped.\r\n", raw);
#endif
        return false;
    }
    return true;
}

void VFS::free_fd(GlobalFileDescriptor fd, ProcessFileDescriptor procfd) {
    auto raw = static_cast<FileDescriptor>(fd);
    auto raw_proc = static_cast<FileDescriptor>(procfd);
    const auto& proc = Scheduler::CurrentProcess->value();

    proc->FileDescriptorTable[raw_proc] = -1;
    proc->FreeFileDescriptors.push_back(raw_proc);
    Opened[raw] = {};
    FreeFileDescriptors.push_back(raw);
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
#ifdef DEBUG_VFS
                    dbgmsg("  Metadata:\r\n"
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
#endif /* #ifdef DEBUG_VFS */
                    if (!metadata.invalid()) {
                        return add_file(std::make_shared<OpenFileDescription>(dev, metadata));
                    }
                }
            }
        }
    }

    return {};
}

bool VFS::close(ProcessFileDescriptor procfd) {
    auto fd = procfd_to_fd(procfd);
    if (fd == GlobalFileDescriptor::Invalid) { return false; }

    auto f = file(fd);
    if (!f) { return false; }

    free_fd(fd, procfd);
    return true;
}

ssz VFS::read(ProcessFileDescriptor fd, u8* buffer, usz byteCount, usz byteOffset) {
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

ssz VFS::write(ProcessFileDescriptor fd, u8* buffer, u64 byteCount, u64 byteOffset) {
    auto f = file(fd);

#ifdef DEBUG_VFS
    dbgmsg("[VFS]: write\r\n"
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
    for (const auto& f : Opened) {
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
    FileDescriptor fd;
    FileDescriptor procfd;

    if (!proc) proc = Scheduler::CurrentProcess->value();

#ifdef DEBUG_VFS
    dbgmsg("[VFS]: Creating file descriptor mapping\r\n");
#endif

    /// Add the file descriptor to the global file table.
/*    if (!FreeFileDescriptors.empty()) {
        fd = FreeFileDescriptors.back();
        FreeFileDescriptors.pop_back();
        Opened[fd] = std::move(file);
#ifdef DEBUG_VFS
        dbgmsg("[VFS]: Reusing freed SysFD %ull\r\n", fd);
#endif
    } else*/

     {
        fd = Opened.size();
        Opened.push_back(std::move(file));
#ifdef DEBUG_VFS
        dbgmsg("[VFS]: Allocating new SysFD %ull\r\n", fd);
#endif
    }

    /// Add the file descriptor to the local process table.
    /*if (!proc->FreeFileDescriptors.empty()) {
        procfd = proc->FreeFileDescriptors.back();
        proc->FreeFileDescriptors.pop_back();
        proc->FileDescriptorTable[procfd] = fd;
#ifdef DEBUG_VFS
        dbgmsg("[VFS]: Reusing freed ProcFD %ull\r\n", procfd);
#endif
    } else*/
    {
        procfd = proc->FileDescriptorTable.size();
        proc->FileDescriptorTable.push_back(fd);
#ifdef DEBUG_VFS
        dbgmsg("[VFS]: Allocating new ProcFD %ull\r\n", procfd);
#endif
    }

#ifdef DEBUG_VFS
    dbgmsg("[VFS]: Mapped ProcFD %ull to SysFD %ull\r\n", procfd, fd);
#endif

    return {static_cast<ProcessFileDescriptor>(procfd), static_cast<GlobalFileDescriptor>(fd)};
}
