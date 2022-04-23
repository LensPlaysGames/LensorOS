#ifndef LENSOR_OS_VIRTUAL_FILESYSTEM_H
#define LENSOR_OS_VIRTUAL_FILESYSTEM_H

#include <cstr.h>
#include <debug.h>
#include <file.h>
#include <filesystem.h>
#include <linked_list.h>
#include <storage/storage_device_driver.h>

// WHAT GOES HERE?
// A path? That wouldn't really be all that efficient,
// but the path could be passed to filesystem driver to read or write from or to files.
// A better option may be to save the StorageDeviceDriver along with byte offset.
struct OpenFileDescription {
    OpenFileDescription(StorageDeviceDriver* driver, u64 byteOffset)
        : Driver(driver), ByteOffset(byteOffset) {}

    StorageDeviceDriver* Driver;
    u64 ByteOffset;
};

struct MountPoint {
    MountPoint(const char* path, Filesystem* fs)
        : Path(path), FS(fs) {}

    const char* Path { nullptr };
    Filesystem* FS { nullptr};
};

class VFS {
public:
    VFS() {}

    void mount(const char* path, Filesystem* fs) {
        Mounts.add(MountPoint(path, fs));
    }

    FileDescriptor open(const char* path, int flags, int mode) {
        /* TODO
         * |-- Parse path.
         * |-- Check beginning of path against each Mounts' Path.
         * |   If it matches, use that Filesystem's functions to proceed.
         * |-- Pass the rest of the path to the filesystem.
         * |-- Store necessary information in a new OpenFileDescription.
         * |   Just path? I dunno.
         * |-- Store new OpenFileDescription in table of descriptions.
         * `-- Return index into table of OpenFileDescriptions as FileDescriptor.
         */
        u64 pathLength = strlen(path);
        if (pathLength <= 1) {
            dbgmsg("Path is not long enough.");
            return -1ull;
        }
        if (path[0] != '/') {
            dbgmsg("%s does not start with slash", path);
            return -1ull;
        }

        FileDescriptor out = -1ull;

        Mounts.for_each([this, &out, path, pathLength](auto* it){
            MountPoint& mount = it->value();
            u64 mountPathLength = strlen(mount.Path);
            if (mountPathLength <= pathLength) {
                if (strcmp(path, mount.Path, mountPathLength)) {
                    // TODO: Get byte offset at path[mountPathLength] from filesystem driver.
                    out = Opened.length();
                    Opened.add_end(OpenFileDescription(mount.FS->storage_device_driver(), 0));
                }
            }
        });
        (void)flags;
        (void)mode;
        return out;
    }

    bool close(FileDescriptor fd) {
        if (fd >= Opened.length())
            return false;

        Opened.remove(fd);
        return true;
    }

    void print_debug() {
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
                   , file.Driver
                   , file.ByteOffset
                   );
            i++;
        });
        dbgmsg("\r\n");
    }

private:
    SinglyLinkedList<OpenFileDescription> Opened;
    SinglyLinkedList<MountPoint> Mounts;
};

#endif /* LENSOR_OS_VIRTUAL_FILESYSTEM_H */
