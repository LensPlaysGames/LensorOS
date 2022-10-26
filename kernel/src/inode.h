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

#ifndef LENSOR_OS_VFS_INODE_H
#define LENSOR_OS_VFS_INODE_H

#include <integers.h>
#include <pure_virtuals.h>

class FileSystem;

/* An Inode, or Index Node, is a unit within a filesystem
 *   that holds the whereabouts of data on a given partition.
 * Inodes can not be transferred from file system to file system due to
 *   potential clash of Inode number, the only identifier classically present.
 * Inode-based file systems can run out of free Inodes, even if more
 *   space is present on the present; this is one major flaw.
 * Inode holds meta-data about the file (permissions, mainly), as well as
 *   the common stuff like status changed, modified, and accessed times.
 *
 * So it seems simple, right? Just have the VFS store a path
 *   in/with/for each inode, and each filesystem can parse the path
 *   accordingly to actually get to the correct data.
 * Caching could be implemented within the VFS on a per-filesystem basis.
 */

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
