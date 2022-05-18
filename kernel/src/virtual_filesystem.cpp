#include <virtual_filesystem.h>

FileDescriptor VFS::open(const String& path) {
    /* Flow
     * |-- Check beginning of path against each Mounts' Path.
     * |   If it matches, use that Filesystem's functions to proceed.
     * |-- Parse path (split on '/') into sections to feed filesystem.
     * |-- Store necessary information in a new OpenFileDescription.
     * |-- Store new OpenFileDescription in table of descriptions.
     * `-- Return index into table of OpenFileDescriptions as FileDescriptor.
     */
    u64 fullPathLength = path.length();
    if (fullPathLength <= 1) {
        dbgmsg_s("Path is not long enough.\r\n");
        return -1ull;
    }
    if (path[0] != '/') {
        dbgmsg("path does not start with slash, %s\r\n", fullPathLength);
        return -1ull;
    }

    FileDescriptor out = -1ull;

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
                    // TODO: path matches a mount path exactly. How do we open a mount? Should we?
                }
                else {
                    FileMetadata metadata = fileDriver->file(dev, prefixlessPath.data());
                    OpenFileDescription openedFile(dev, metadata);
                    out = Opened.length();
                    Opened.add_end(openedFile);
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

bool VFS::read(FileDescriptor fd, u8* buffer, u64 byteCount, u64 byteOffset) {
    if (fd >= Opened.length())
        return false;

    OpenFileDescription& file = Opened[fd];
    file.DeviceDriver->read(file.Metadata.byte_offset() + byteOffset
                            , byteCount, buffer);
    return true;
}

bool VFS::write(FileDescriptor fd, u8* buffer, u64 byteCount, u64 byteOffset) {
    if (fd >= Opened.length())
        return false;

    OpenFileDescription& file = Opened[fd];
    file.DeviceDriver->write(byteOffset, byteCount, buffer);
    return true;
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
