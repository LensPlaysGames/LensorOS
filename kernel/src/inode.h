#ifndef LENSOR_OS_VFS_INODE_H
#define LENSOR_OS_VFS_INODE_H

#include "pure_virtuals.h"
#include "integers.h"

class FileSystem;

struct InodeStat {
    u64 DeviceID;
    u64 InodeNumber;
    u64 Mode;
    u64 LinkCount;
    u64 UserID;
    u64 GroupID;
    u64 SpecialDeviceID;
    u64 SizeInBytes;
    u64 BlockSize;  // Block size for filesystem I/O.
    u64 BlockCount; // Number of 512-byte blocks.
    s64 AccessedSeconds;
    u64 AccessedNanoseconds;
    s64 ModifiedSeconds;
    u64 ModifiedNanoseconds;
    s64 ChangedSeconds;
    u64 ChangedNanoseconds;
};

/// An Inode, or Index Node, is a unit within a filesystem
///   that holds the whereabouts of data on a given partition.
/// Inode's can not be transferred from file system to file system due to
///   potential clash of Inode number, the only identifier present.
/// Inode-based file systems can run out of free Inodes, even if more
///   space is present on the disk. Luckily for the VFS it's never on disk.
/// Inode holds meta-data about the file (permissions, mainly), as well as
///   the common stuff like creation, modified, and accessed times.

/*
My problem: 
|- FAT can not convert an inode index into a resolved file.
|- The FAT has to be traversed according to a given path.
|- For example: "/usr/textfile.txt" would search root directory
|    for "usr" directory, then if that directory is found,
|    search it for a file named "textfile" with a "txt" extension.
|
|-So it seems simple, right? 
| `- Just have the VFS store a path in each inode, and each filesystem 
|    | can parse the path accordingly to actually get to the correct data.
|    `- Yes, but how does an inode-based filesystem get an index from a file path?
|       `- I don't know at all.
*/

// TODO:
// `- Implement a/c/m/d time and hard-link count.
class Inode {
public:
    Inode(FileSystem& fs, u64 i)
        : Filesystem(fs)
        , Index(i) {}

    FileSystem& get_filesystem() { return Filesystem; }
    u64 get_index()              { return Index;      }

    /// Get a child inode from this directory inode by a given null-terminated relative path.
    /// To be implemented by each file system's custom Inode class.
    /// This will allow even non inode-based filesystems to work with the VFS.
    virtual Inode* lookup(char* path) { (void)path; return nullptr; };
private:
    /// Identifiers
    FileSystem& Filesystem;
    u8* Path { nullptr };
    u64 Index        { 0 };
    /// Metadata
    u64 SizeInBytes  { 0 };
    u64 Mode         { 0 }; // File attributes.
    u64 UserID       { 0 }; // ID of User that owns this inode.
    u64 GroupID      { 0 }; // ID of Group that owns this inode.
    u64 LinkCount    { 0 }; // Number of hard-links to this inode.
    s64 AccessedTime { 0 };
    s64 ChangedTime  { 0 };
    s64 ModifiedTime { 0 };
    s64 DeletionTime { 0 };
    u64 BlockSize    { 0 }; // Block size for Filesystem I/O.
    u64 BlockCount   { 0 }; // Number of 512-byte blocks.
};

#endif
