#ifndef LENSOR_OS_VIRTUAL_FILESYSTEM_H
#define LENSOR_OS_VIRTUAL_FILESYSTEM_H

#include <cstr.h>
#include <file.h>
#include <linked_list.h>
#include <system.h>
#include <uart.h>

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
        Opened.add(OpenFileDescription(nullptr, 0));
        return 0;
    }

    void print_debug() {
        UART::out("[VFS]: Information Dump\r\n"
                  "  Mounts:\r\n");
        u64 i = 0;
        Mounts.for_each([&i](auto* it) {
            MountPoint& mp = it->value();
            UART::out("    Mount ");
            UART::out(i);
            UART::out(":\r\n"
                      "      Path: ");
            UART::out(mp.Path);
            UART::out("\r\n"
                      "      Filesystem: ");
            UART::out(Filesystem::type2name(mp.FS->type()));
            UART::out("\r\n"
                      "        Filesystem Driver Address: 0x");
            UART::out(to_hexstring(mp.FS->filesystem_driver()));
            UART::out("\r\n"
                      "        Storage Device Driver Address: 0x");
            UART::out(to_hexstring(mp.FS->storage_device_driver()));
            UART::out("\r\n");
            i += 1;
        });
        i = 0;
        UART::out("\r\n"
                  "  Filesystems:\r\n");
        Opened.for_each([&i](auto* it){
            OpenFileDescription& file = it->value();
            UART::out("    Storage Device Driver Address: 0x");
            UART::out(to_hexstring(file.Driver));
            UART::out("\r\n"
                      "    Byte Offset: ");
            UART::out(file.ByteOffset);
        });
        UART::out("\r\n");
    }

private:
    // FIXME: This should definitely be more
    //        efficient than a singly linked list...
    SinglyLinkedList<OpenFileDescription> Opened;
    SinglyLinkedList<MountPoint> Mounts;
};

#endif /* LENSOR_OS_VIRTUAL_FILESYSTEM_H */
