#ifndef LENSOR_OS_VIRTUAL_FILESYSTEM_H
#define LENSOR_OS_VIRTUAL_FILESYSTEM_H

#include <cstr.h>
#include <debug.h>
#include <file.h>
#include <filesystem.h>
#include <linked_list.h>
#include <storage/file_metadata.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <string.h>

struct OpenFileDescription {
    OpenFileDescription(StorageDeviceDriver* driver, const FileMetadata& md)
        : DeviceDriver(driver), Metadata(md) {}

    StorageDeviceDriver* DeviceDriver { nullptr };
    FileMetadata Metadata;
};

struct MountPoint {
    MountPoint(const char* path, Filesystem* fs)
        : Path(path), FS(fs) {}

    const char* Path { nullptr };
    Filesystem* FS { nullptr };
};

class VFS {
public:
    VFS() {}

    void mount(const char* path, Filesystem* fs) {
        Mounts.add(MountPoint(path, fs));
    }

    FileDescriptor open(const String& path);
    FileDescriptor open(const char* path) {
        return open(String(path));
    }

    bool close(FileDescriptor fd);

    bool read(FileDescriptor fd, u8* buffer, u64 byteCount, u64 byteOffset = 0);
    bool write(FileDescriptor fd, u8* buffer, u64 byteCount, u64 byteOffset);

    void print_debug();

private:
    SinglyLinkedList<OpenFileDescription> Opened;
    SinglyLinkedList<MountPoint> Mounts;
};

#endif /* LENSOR_OS_VIRTUAL_FILESYSTEM_H */
