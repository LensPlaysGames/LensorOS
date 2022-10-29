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
#include <debug.h>
#include <file.h>
#include <filesystem.h>
#include <linked_list.h>
#include <storage/file_metadata.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <string.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_VFS

FileDescriptor VFS::open(const String& path) {
    u64 fullPathLength = path.length();
    if (fullPathLength <= 1) {
        dbgmsg_s("[VFS]: path is not long enough.\r\n");
        return (FileDescriptor)-1;
    }
    if (path[0] != '/') {
        dbgmsg("[VFS]: path does not start with slash, %s\r\n", fullPathLength);
        return (FileDescriptor)-1;
    }
    FileDescriptor out = (FileDescriptor)-1;
    Mounts.for_each([this, &out, path, fullPathLength](auto* it) {
        MountPoint& mount = it->value();
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
                    if (metadata.invalid() == false) {
                        OpenFileDescription openedFile(dev, metadata);
                        out = Opened.length();
                        Opened.add_end(openedFile);
                    }
                }
            }
        }
    });
    return out;
}

bool VFS::close(FileDescriptor fd) {
    if (fd >= Opened.length())
        return false;

    Opened.remove(fd);
    return true;
}

ssz VFS::read(FileDescriptor fd, u8* buffer, usz byteCount, usz byteOffset) {
    if (fd >= Opened.length())
        return -1;

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

    OpenFileDescription& file = Opened[fd];
    return file.DeviceDriver->read
        (file.Metadata.byte_offset() + byteOffset, byteCount, buffer);
}

ssz VFS::write(FileDescriptor fd, u8* buffer, u64 byteCount, u64 byteOffset) {
    if (fd >= Opened.length())
        return -1;

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

    OpenFileDescription& file = Opened[fd];
    return file.DeviceDriver->write(file.Metadata.byte_offset() + byteOffset, byteCount, buffer);
}

void VFS::print_debug() {
    dbgmsg("[VFS]: Debug Info\r\n"
           "  Mounts:\r\n");
    u64 i = 0;
    Mounts.for_each([&i](auto* it) {
        MountPoint& mp = it->value();
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
    });
    dbgmsg("\r\n"
           "  Opened files:\r\n");
    i = 0;
    Opened.for_each([&i](auto* it){
        OpenFileDescription& file = it->value();
        dbgmsg("    Open File %ull:\r\n"
               "      Storage Device Driver Address: %x\r\n"
               "      Byte Offset: %ull\r\n"
               , i
               , file.DeviceDriver
               , file.Metadata.byte_offset()
               );
        i++;
    });
    dbgmsg("\r\n");
}

void VFS::add_file(OpenFileDescription fileDescription) {
    Opened.add_end(fileDescription);
}
