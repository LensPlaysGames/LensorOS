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

#include <virtual_filesystem.h>

#include <cstr.h>
#include <format>
#include <storage/file_metadata.h>
#include <scheduler.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_VFS

#ifdef DEBUG_VFS
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...)
#endif

SysFD VFS::procfd_to_fd(ProcFD procfd) const {
    return procfd_to_fd(Scheduler::CurrentProcess->value(), procfd);
}

SysFD VFS::procfd_to_fd(Process* process, ProcFD procfd) const {
    auto sysfd = process->FileDescriptors[procfd];
    if (!sysfd) {
        DBGMSG("[VFS]: ERROR {} (pid {}) is unmapped.\n", procfd, process->ProcessID);
        return SysFD::Invalid;
    }

    return *sysfd;
}

auto VFS::file(ProcFD procfd) -> std::shared_ptr<FileMetadata> {
    // TODO: We should probably have the implementation take a process
    // as a parameter, that way we can actually free fds other than
    // within the currently scheduled process. :p
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
        std::print("[VFS]: ERROR: {} (pid {}) is unmapped.\n", procfd, proc->ProcessID);
        return {};
    }

    DBGMSG("[VFS]: file: {} ({}) is mapped to SysFD {}.\n", procfd, proc->ProcessID, *sysfd);
    return file(static_cast<SysFD>(*sysfd));
}

auto VFS::file(SysFD fd) -> std::shared_ptr<FileMetadata> {
    auto f = Files[fd];
    if (!f) {
        std::print("[VFS]: ERROR: {} is unmapped.\n", fd);
        return {};
    }
    return f;
}

bool VFS::valid(Process *proc, ProcFD procfd) const {
    return procfd_to_fd(proc, procfd) != SysFD::Invalid;
}

bool VFS::valid(ProcFD procfd) const {
    return procfd_to_fd(Scheduler::CurrentProcess->value(), procfd) != SysFD::Invalid;
}

bool VFS::valid(SysFD fd) const {
    auto f = Files[fd];
    if (!f) {
        std::print("[VFS]: ERROR: {} is unmapped.\n", fd);
        return false;
    }
    return true;
}

/// Erasing the last shared_ptr holding the file metadata will call
/// the destructor of FileMetadata, which will then close the file.
void VFS::free_fd(Process* process, SysFD fd, ProcFD procfd) {
    if (!process) return;
    DBGMSG("Freeing ProcFD={} SysFD={} in process {}\n", procfd, fd, process->ProcessID);
    // Remove file descriptor from process's list of open file
    // descriptors using Process File Descriptor.
    process->FileDescriptors.erase(procfd);
    // Remove kernel file description from VFS list of open files using
    // System File Descriptor.
    Files.erase(fd);
}

void VFS::free_fd(SysFD fd, ProcFD procfd) {
    free_fd(Scheduler::CurrentProcess->value(), fd, procfd);
}

FileDescriptors VFS::open(std::string_view path) {
    u64 fullPathLength = path.size();

    if (fullPathLength <= 1) {
        std::print("[VFS]: path is not long enough.\n");
        return {};
    }

    if (path[0] != '/') {
        std::print("[VFS]: path does not start with slash, {}\n", fullPathLength);
        return {};
    }

    DBGMSG("[VFS]: Attempting to open file at path {}\n", path);
    for (const auto& mount : Mounts) {
        /// It makes no sense to search file systems whose mount point does not
        /// match the beginning of the path. And even if they’re mounted twice,
        /// we’ll still find the second mount.
        if (!path.starts_with(mount.Path)) continue;
        auto fs_path = path.substr(mount.Path.size());

        /// Try to open the file.
        DBGMSG("[VFS]: Attempting to open file at path {} on mount {}\n", fs_path, mount.Path);
        if (auto meta = mount.FS->open(fs_path)) {
            DBGMSG("  Metadata:\n"
                   "    Name: {}\n"
                   "    File Size: {}\n"
                   "    Driver Data: {}\n"
                   "    Device Driver: {}\n"
                   "    Invalid: {}\n"
                   , meta->name()
                   , meta->file_size()
                   , meta->driver_data()
                   , (void*) meta->device_driver().get()
                   , meta->invalid()
            );
            return add_file(std::move(meta));
        }
    }

    return {};
}


bool VFS::close(Process* process, ProcFD procfd) {
    if (!process) return false;
    auto fd = procfd_to_fd(process, procfd);
    if (fd == SysFD::Invalid) {
        DBGMSG("[VFS]: Cannot close invalid {} (pid {}).\n", procfd, process->ProcessID);
        return false;
    }

    auto f = file(fd);
    if (!f) {
        DBGMSG("[VFS]: Cannot close invalid {}.\n", fd);
        return false;
    }

    DBGMSG("[VFS]: Unmapping {} (pid {})  \"{}\".\n", procfd, process->ProcessID, f->name());
    free_fd(process, fd, procfd);
    return true;
}

bool VFS::close(ProcFD procfd) {
    return close(Scheduler::CurrentProcess->value(), procfd);
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

    // Scheduler::yield() is noreturn, but that doesn't mean the stackframe(s)
    // will be cleaned up. So we either have to
    //   A. not take the shared_ptr here, and instead a weak_ptr,
    //      possibly risking a race condition of the file being closed
    //      while it's being read from (not good), or
    //   B. Take a shared ptr but instead of passing it, move it to the
    //      device driver. This would mean the device driver could unlock
    //      it or whatever before yielding, or
    //   C. Have the device driver `read()` function return a value
    //      that indicates whether or not we should yield; take the
    //      shared_ptr in a nested scope, call read, then outside of
    //      that scope, conditionally call yield.
    FileMetadata* meta = nullptr;
    {
        auto f = file(fd);
        meta = f.get();
    }
    if (!meta) return -1;

    DBGMSG("  file offset:       {}\n", meta->offset);

    return meta->device_driver()->read(meta, byteOffset + meta->offset, byteCount, buffer);
}

ssz VFS::write(ProcFD fd, u8* buffer, u64 byteCount, u64 byteOffset) {
    /*
    DBGMSG("[VFS]: write11111111111111111\n"
           "  file descriptor: {}\n"
           "  buffer address:  {}\n"
           "  byte count:      {}\n"
           "  byte offset:     {}\n"
           , fd
           , (void*) buffer
           , byteCount
           , byteOffset
           );
    */

    // SEE COMMENTS ON CONCURRENCY AND (B)LOCKING IN VFS::read()
    FileMetadata* meta = nullptr;
    {
        auto f = file(fd);
        meta = f.get();
    }
    if (!meta) return -1;

    DBGMSG("[VFS]: write\n"
           "  name:            {}\n"
           "  file descriptor: {}\n"
           "  buffer address:  {}\n"
           "  byte count:      {}\n"
           "  byte offset:     {}\n"
           "  file offset:     {}\n"
           , meta->name()
           , fd
           , (void*) buffer
           , byteCount
           , byteOffset
           , meta->offset
           );

    return meta->device_driver()->write(meta, byteOffset + meta->offset, byteCount, buffer);
}

void VFS::print_debug() {
    std::print("[VFS]: Debug Info\n"
           "  Mounts:\n");
    u64 i = 0;
    for (const auto& mp : Mounts) {
        std::print("    Mount {}:\n"
                   "      Path: {}\n"
                   "      Filesystem: {}\n"
                   "      Driver Address: {}\n"
                   , i
                   , mp.Path
                   , mp.FS->name()
                   , (void*) mp.FS.get()
                   );
        i += 1;
    }
    std::print("\n  Opened files:\n");
    i = 0;
    for (const auto& f : Files) {
        std::print("    Open File {}:\n"
                   "      Driver Address: {}\n"
                   , i
                   , (void*) f->device_driver().get()
        );
        i++;
    }
    std::print("\n");
}

FileDescriptors VFS::add_file(std::shared_ptr<FileMetadata> file, Process* proc) {
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
    return { static_cast<ProcFD>(procfd), static_cast<SysFD>(fd) };
}
