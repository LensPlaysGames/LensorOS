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

#include <fat_definitions.h>
#include <fat_directory_iterator.h>
#include <integers.h>
#include <storage/file_metadata.h>
#include <storage/filesystem_drivers/file_allocation_table.h>

#include <format>
#include <memory>
#include <string>
#include <vector>

// Uncomment the following directive for extra debug information output.
// #define DEBUG_FAT

#ifdef DEBUG_FAT
#define DBGMSG(...) std::print(__VA_ARGS__)
#else
#define DBGMSG(...)
#endif

std::string FileAllocationTableDriver::pop_filename_from_front_of_path(std::string& raw_path) {
    /// Strip leading slash.
    std::string path = raw_path;
    if (path.starts_with("/")) path = path.substr(1);
    // If there was nothing past the leading slash, return the leading slash.
    if (path.size() < 1) return raw_path;

    raw_path = path;
    size_t first_sep = path.find_first_of("/");
    if (first_sep == std::string::npos) return path;

    std::string out = path.substr(0, first_sep);
    raw_path = path.substr(first_sep);
    // Special handling of trailing directory separator.
    if (raw_path == "/") raw_path = out;
    return out;
}

void FileAllocationTableDriver::print_fat(BootRecord& br) {
    std::print(
        "File Allocation Table Boot Record:\n"
        "  Total Clusters:      {}\n"
        "  Sectors / Cluster:   {}\n"
        "  Total Sectors:       {}\n"
        "  Bytes / Sector:      {}\n"
        "  Sectors / FAT:       {}\n"
        "  Sector Offsets:\n"
        "    FATs:      {}\n"
        "    Data:      {}\n"
        "    Root Dir.: {}\n"
        "\n",
        br.total_clusters(),
        br.BPB.NumSectorsPerCluster,
        br.BPB.total_sectors(),
        u16(br.BPB.NumBytesPerSector),
        br.fat_sectors(),
        br.BPB.first_fat_sector(),
        br.first_data_sector(),
        br.first_root_directory_sector());
}

FATType FileAllocationTableDriver::fat_type(BootRecord& br) {
    u64 totalClusters = br.total_clusters();
    if (totalClusters == 0)
        return FATType::ExFAT;
    else if (totalClusters < 4085)
        return FATType::FAT12;
    else if (totalClusters < 65525)
        return FATType::FAT16;
    else
        return FATType::FAT32;
}

auto FileAllocationTableDriver::try_create(std::shared_ptr<StorageDeviceDriver> driver) -> std::shared_ptr<FilesystemDriver> {
    if (!driver) return nullptr;

    BootRecord br;
    auto n_read = driver->read_raw(0, sizeof br, &br);
    if (n_read != sizeof br) {
        std::print("Failed to read boot record from device\n");
        return nullptr;
    }

    u64 totalSectors = br.BPB.TotalSectors16 == 0
                           ? br.BPB.TotalSectors32
                           : br.BPB.TotalSectors16;

    /* Validate boot sector is of FAT format.
     * TODO: Use more of these confidence checks before
     *       assuming it is valid FAT filesystem.
     * What makes a FAT filesystem valid?
     * Thanks to Gigasoft of osdev forums for this list.
     * [x] = something this driver checks.
     * [x] Word at byte offset 510 equates to 0xaa55.
     * [x] Sector size is power of two between 512-4096 (inclusive).
     * [x] Cluster size is a power of two.
     * [ ] Media type is 0xf0 or greater or equal to 0xf8.
     * [ ] FAT size is not zero.
     * [x] Number of sectors is not zero.
     * [ ] Number of root directory entries is:
     *       - zero if FAT32.
     *       - not zero if FAT12 or FAT16.
     * [ ] (FAT32) Root cluster is valid
     * [ ] (FAT32) File system version is zero
     * [x] NumFATsPresent greater than zero
     */
    bool out = (br.Magic == 0xaa55
                && totalSectors != 0
                && br.BPB.NumBytesPerSector >= 512
                && br.BPB.NumBytesPerSector <= 4096
                && (br.BPB.NumBytesPerSector & (br.BPB.NumBytesPerSector - 1)) == 0
                && (br.BPB.NumSectorsPerCluster & (br.BPB.NumSectorsPerCluster - 1)) == 0
                && br.BPB.NumFATsPresent > 0);

    if (not out) return nullptr;

#ifdef DEBUG_FAT
    print_fat(br);
#endif /* DEBUG_FAT */

    auto fs = std::make_shared<FileAllocationTableDriver>(
        std::move(driver),
        std::move(br));
    fs->This = fs;
    return std::static_pointer_cast<FilesystemDriver>(fs);
}

// This needs to match whatever form is returned from the directory
// iterator, basically, so that we can actually do comparisons between the
// return value and actual files' names.
// "abcdefgh.ijk" needs to become "ABCDEFGH.IJK"
// "ABCDEFGHIJK" needs to become "ABCDEFGHIJK"
// "blazeit" needs to become "BLAZEIT"
auto FileAllocationTableDriver::translate_filename(std::string_view raw_filename) -> std::string {
    std::string path = raw_filename;

    // toupper
    for (usz i = 0; i < path.size(); ++i)
        if (path[i] >= 97 and path[i] <= 122) path[i] -= 32;

    return path;
}

void FileAllocationTableDriver::read_cluster_into(std::vector<u8>& out, u32 cluster_index) {
    // Cluster Index Out Of Bounds
    if (cluster_index < 2 or cluster_index > BR.total_clusters()) {
        std::print("[FAT]: cluster index out of bounds\n");
        return;
    }

    const auto cluster_sector = BR.cluster_to_sector(cluster_index);

    // Cluster Sector Out Of Bounds
    if (cluster_sector > BR.BPB.total_sectors()) {
        std::print("[FAT]: cluster sector out of bounds\n");
        return;
    }
    const auto sector_byte_offset = cluster_sector * sector_size();

    const auto buffer_begin = out.size();
    out.resize(buffer_begin + cluster_size());
    Device->read_raw(
        sector_byte_offset,
        cluster_size(),
        out.data() + buffer_begin);
}

std::vector<u8> FileAllocationTableDriver::read_cluster(u32 cluster_index) {
    auto cluster_contents = std::vector<u8>(BR.BPB.cluster_size());
    read_cluster_into(cluster_contents, cluster_index);
    return cluster_contents;
}

u32 FileAllocationTableDriver::traverse_path_for_cluster(std::string_view raw_path, u32 directory_cluster) {
    std::string path(raw_path);
    auto raw_filename = pop_filename_from_front_of_path(path);
    auto filename = translate_filename(raw_filename);
    auto traversal = FAT::FileTraversal(*this, directory_cluster);
    for (const auto& Entry : traversal.go_directory()) {
        // Skip unrelated entries.
        if (Entry.name != filename) continue;

        // From this point on, we know we are dealing with an entry that refers to
        // the front component of the path that was just popped off.

        // If path and raw_filename are equal, we can not resolve any more
        // filenames from full path; we have found the file the path points to.
        // std::print("path:\"{}\" | raw_filename:\"{}\" | filename:\"{}\" \n", path, raw_filename, filename);
        if (path == raw_filename) {
            // Return the file's stored cluster index. This is where the file's
            // metadata and data can be found.
            return Entry.cluster_number;
        }

        // Otherwise, there is more in the path to traverse, and we've just
        // matched a part from the beginning. We need to further recurse into this
        // directory; if it isn't a directory, then the path doesn't make sense
        // and we error out.
        if (!Entry.directory) return -1;

        // Recurse into directory...
        return traverse_path_for_cluster(path, Entry.cluster_number);
    }
    // Didn't find front component of path in directory pointed to by given
    // directory cluster.
    return -1;
}

std::shared_ptr<FileMetadata> FileAllocationTableDriver::traverse_path(
    std::string_view raw_path,
    u32 directory_cluster) {
    DBGMSG(
        "[FAT]: traverse_path(): raw_path=\"{}\", directory_cluster={:#x}\n",
        raw_path,
        directory_cluster);

    /// Strip leading slash.
    if (raw_path.starts_with("/")) {
        // FIXME: If it's just a slash, return the root directory.
        if (raw_path.size() == 1) {
            // return FileMetadata::Make(FileMetadata::FileType::Directory,
            //                           raw_path,
            //                           fsd(This.lock()),
            //                           0,
            //                           0);
        }
        raw_path = raw_path.substr(1);
    }
    if (raw_path.size() < 1) {
        DBGMSG("[FAT]:open(): Invalid path: {}\n", raw_path);
        return {};
    }

    // Get first filename from path.
    // Given path "foo/bar/bas.exe", return "foo" as a legal FAT
    // filename, and alter given path to be "past" that + directory
    // separator.
    std::string path = raw_path;
    auto raw_filename = pop_filename_from_front_of_path(path);
    DBGMSG("[FAT]:open(): Got filename \"{}\" and path \"{}\" from \"{}\"\n", raw_filename, path, raw_path);

    // Translate path (FAT has very limited file names).
    std::string filename = translate_filename(raw_filename);
    DBGMSG("[FAT]:open(): Translated filename \"{}\" from \"{}\"\n", filename, raw_filename);

    auto traversal = FAT::FileTraversal(*this, directory_cluster);
    for (const auto& Entry : traversal.go_directory()) {
        DBGMSG("[FAT]: name:\"{}\"\n", Entry.name);
        if (Entry.name != filename) continue;
        // If path and raw_filename are equal, we can not resolve any more
        // filenames from full path; we have found the file.
        if (path == raw_filename) {
            DBGMSG(
                "  Found file at {}!\n"
                "    Short: \"{}\"\n"
                "    Name: \"{}\"\n",
                path,
                std::string_view((const char*)Entry.short_name.data(), Entry.short_name.size()),
                Entry.name);
            FileMetadata::FileType ftype = Entry.directory
                                               ? FileMetadata::FileType::Directory
                                               : FileMetadata::FileType::Regular;
            return FileMetadata::Make(ftype,
                                      std::move(filename),
                                      fsd(This.lock()),
                                      u32(Entry.file_size_in_bytes),
                                      (void*)(uintptr_t)(Entry.cluster_number));
        }

        // Otherwise, we need to recurse into the directory.
        if (not Entry.directory) {
            std::print("[FAT]: Cannot follow path \"{}\" because \"{}\" is not a directory\n", path, filename);
            return {};
        }

        // Recurse into directory...
        u32 dirCluster = Entry.cluster_number;
        // std::print("Recursing! Following {} at cluster {}\n", path, dirCluster);
        return traverse_path(path, dirCluster);
    }

    /// No such file.
    std::print("[FAT]: Could not find file at \"{}\", sorry\n", filename);
    return {};
}

auto FileAllocationTableDriver::open(std::string_view raw_path) -> std::shared_ptr<FileMetadata> {
    DBGMSG("[FAT]: Attempting to open file {}\n", raw_path);
    if (Device == nullptr) {
        /// Should never get here.
        std::print("[FAT]::open(): Device is null!\n");
        return {};
    }

#ifdef DEBUG_FAT
    auto __this = This.lock();
    if (!__this) {
        /// Should never get here.
        std::print("[FAT]::open(): `This` is null!\n");
        return {};
    }
#endif

    // TODO: Create file if flags ask for it and no existing file is found.
    return traverse_path(raw_path);
}

ssz FileAllocationTableDriver::directory_data(std::string_view path, usz max_entry_count, DirectoryEntry* out) {
    if (not max_entry_count) return 0;
    if (not out) return -1;

    // Basically, we are doing a path traversal but not ever opening the files
    // we encounter, instead building dir entries corresponding to them.

    // Begin with traversing root directory
    u32 directory_cluster = BR.sector_to_cluster(BR.first_root_directory_sector());

    // If path isn't empty and isn't root, traverse path and ensure we end up
    // in a directory.
    // FIXME: This is basically three-quarters of `traverse_path`, but it
    // doesn't return a FileMetadata, just a directory cluster number. Could
    // abstract.
    if (path.size() and path != std::string_view("/")) {
        u32 new_directory_cluster = traverse_path_for_cluster(path, directory_cluster);
        if (new_directory_cluster == u32(-1)) return -1;
        directory_cluster = new_directory_cluster;
    }

    ssz count = 0;
    auto traversal = FAT::FileTraversal(*this, directory_cluster);
    for (const auto& Entry : traversal.go_directory()) {
        // Skip volume label(s).
        if (Entry.volume_id) continue;

        // Copy file name into entry name.
        memcpy(
            &out[count].name[0],
            Entry.name.data(),
            std::min(Entry.name.size(), sizeof(out[count].name)));

        // Set directory vs regular file type.
        out[count].type = Entry.directory
                              ? FileMetadata::FileType::Directory
                              : FileMetadata::FileType::Regular;

        // Ensure we don't write too many entries.
        if (usz(++count) >= max_entry_count) break;
    }

    return count;
}

ssz FileAllocationTableDriver::read(FileMetadata* file, usz offset, usz size, void* buffer, usz flags) {
    DBGMSG("[FAT]: read() file:{} offset:{} size:{}\n", file->name(), offset, size);

    auto traversal = FAT::FileTraversal(*this, usz(file->driver_data()));
    auto data_iterator = traversal.go_file_data();
    const auto offset_cluster_index = offset / cluster_size();
    const auto offset_within_cluster = offset % cluster_size();

    // Skip past one cluster at a time until we hit the cluster with the
    // requested offset within it.
    for (usz i = 0; i < offset_cluster_index; ++i) {
        // EOF before offset reached
        if (data_iterator == data_iterator.end()) return 0;
        ++data_iterator;
    }

    // First cluster with requested data in it; copy offset modulo cluster
    // size bytes, adjust size, then do the rest of the clusters except for
    // the last. This is because the requested data may reside in the middle
    // of an actual cluster.
    auto& first_cluster = *data_iterator;
    ++data_iterator;
    auto first_cluster_size = std::min(cluster_size() - offset_within_cluster, size);
    memcpy(
        buffer,
        first_cluster.data() + offset_within_cluster,
        first_cluster_size);
    uintptr_t bytes_read = first_cluster_size;

    if (bytes_read == size) return size;

    auto remaining_full_clusters = (size - bytes_read) / cluster_size();
    for (usz i = 0; i < remaining_full_clusters; ++i) {
        if (data_iterator == data_iterator.end())
            return bytes_read;
        auto& cluster = *data_iterator;
        ++data_iterator;
        memcpy(((uint8_t*)buffer) + bytes_read,
               cluster.data(),
               cluster_size());
        bytes_read += cluster_size();
    }

    if (bytes_read == size) return size;

    auto& last_cluster = *data_iterator;
    auto last_cluster_size = size - bytes_read;
    memcpy(
        ((uint8_t*)buffer) + bytes_read,
        last_cluster.data(),
        last_cluster_size);

    return size;
}

void FileAllocationTableDriver::fresh_fat(std::vector<u8>& FAT) {
    const u64 FAToffset = first_fat_sector() * sector_size();
    const u64 FATsize = fat_sector_count() * sector_size();
    FAT.resize(0);
    FAT.resize(FATsize);
    Device->read_raw(
        FAToffset,
        FATsize,
        FAT.data());
}
