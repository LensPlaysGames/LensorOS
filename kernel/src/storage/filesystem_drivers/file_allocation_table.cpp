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

struct FATFileData {
    u32 first_cluster;
    u32 parent_first_cluster;
    u32 parent_entry_cluster;
    u32 entry_index_within_cluster;
};

static auto make(
    FileType type,
    std::string name,
    std::shared_ptr<FilesystemDriver> driver,
    usz size,
    u32 first_cluster,
    u32 parent_first_cluster,
    u32 parent_entry_cluster,
    u32 entry_index_within_cluster) {
    auto* data = new FATFileData;
    data->first_cluster = first_cluster;
    data->parent_first_cluster = parent_first_cluster;
    data->parent_entry_cluster = parent_entry_cluster;
    data->entry_index_within_cluster = entry_index_within_cluster;
    return FileMetadata::Make(
        type,
        std::move(name),
        driver,
        size,
        data);
}

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

auto FileAllocationTableDriver::try_create(std::shared_ptr<StorageDeviceDriver> driver)
    -> std::shared_ptr<FilesystemDriver> {
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

    const auto extension_separator = raw_filename.find_last_of(".");
    auto name = raw_filename.substr(0, extension_separator);
    auto extension = raw_filename.substr(
        extension_separator != std::string::npos
            ? extension_separator + 1
            : std::string::npos);

    // Don't touch Long File Names (LFNs)
    if (not ShortFileNameEntry::fits(name, extension))
        return path;

    // toupper
    for (usz i = 0; i < path.size(); ++i)
        if (path[i] >= 97 and path[i] <= 122) path[i] -= 32;

    return path;
}

void FileAllocationTableDriver::read_cluster_into(std::vector<u8>& out, u32 cluster_index) {
    // Cluster Index Out Of Bounds
    if (cluster_index < 2 or cluster_index > BR.total_clusters()) {
        std::print(
            "[FAT]: cluster index out of bounds ({} out of {})\n",
            cluster_index,
            BR.total_clusters());
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
    if (directory_cluster == decltype(directory_cluster)(-1))
        directory_cluster = root_directory_cluster();

    if (raw_path == "/")
        return directory_cluster;

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

        if (Entry.cluster_number == 0) break;

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

    if (directory_cluster == u32(-1))
        directory_cluster = root_directory_cluster();

    /// Strip leading slash.
    if (raw_path.starts_with("/")) {
        // FIXME: If it's just a slash, return the root directory.
        if (raw_path.size() == 1) {
            // return make(FileMetadata::FileType::Directory,
            //             raw_path,
            //             fsd(This.lock()),
            //             0,
            //             directory_cluster,
            //             -1,
            //             -1,
            //             -1);
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
            return make(
                ftype,
                std::move(filename),
                fsd(This.lock()),
                u32(Entry.file_size_in_bytes),
                Entry.cluster_number,
                directory_cluster,
                Entry.within_cluster,
                Entry.entry_index);
        }

        // Otherwise, we need to recurse into the directory.
        if (not Entry.directory) {
            std::print("[FAT]: Cannot follow path \"{}\" because \"{}\" is not a directory\n", path, filename);
            return {};
        }

        // Recurse into directory...
        const u32 dirCluster = Entry.cluster_number;

        // NOTE: Empty directories /may/ have no cluster's actually allocated to
        // them (the directory entry within their parent directory is the only
        // thing that "makes them exist").
        if (dirCluster == 0) break;

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

    auto existing = traverse_path(raw_path);
    if (existing) {
        DBGMSG("[FAT]::open(): Found existing file at \"{}\"\n", raw_path);
        return existing;
    }
    DBGMSG("[FAT]::open(): Creating new file at \"{}\"\n", raw_path);

    // File opened at path does not exist; create new file at path.
    std::vector<u8> FAT{};
    fresh_fat(FAT);
    // Find free directory entry in directory; er, we may need to find a
    // free cluster for the directory as well, if it is out of free directory
    // entries entirely.

    // TODO: Directory or Regular?
    const auto ftype = FileType::Regular;

    // 1. Get first cluster of parent directory; create directory entry
    //    iterator on this directory.
    const auto last_separator = raw_path.find_last_of("/") + 1;
    auto parent = raw_path.substr(0, last_separator);
    auto filename = raw_path.substr(last_separator);
    DBGMSG(
        "  last_separator: {}\n"
        "  parent:\"{}\" filename:\"{}\"\n",
        last_separator,
        parent,
        filename);

    const auto extension_separator = filename.find_last_of(".");
    auto filename_name = filename.substr(0, extension_separator);
    auto filename_extension = filename.substr(
        extension_separator != std::string::npos
            ? extension_separator + 1
            : std::string::npos);
    DBGMSG(
        "  bare-name:\"{}\" extension:\"{}\"\n",
        filename_name,
        filename_extension);

    auto parent_cluster = traverse_path_for_cluster(parent, -1);

    if (parent_cluster == u32(-1)) {
        std::print("[FAT]::open(): Could not get parent directory \"{}\" for new file at \"{}\"\n", parent, raw_path);
        return {};
    }

    if (parent_cluster == 0) {
        std::print("[FAT]::open(): TODO: Handle allocating first cluster for empty directory {}\n", parent);
        return {};
    }

    // 2. Calculate how many contiguous free entries we need for this file,
    //    *including long file name entries*.
    bool fits_in_short_filename = filename_name.size() <= 8;
    if (fits_in_short_filename) {
        char translated_short_name_buffer[8];
        memcpy(&translated_short_name_buffer[0], filename_name.data(), filename_name.size());
        FAT::replace_banned_chars_with(
            {&translated_short_name_buffer[0], filename_name.size()},
            '_');
        auto translated_short_name = std::string_view(translated_short_name_buffer, filename_name.size());
        fits_in_short_filename = ShortFileNameEntry::fits(translated_short_name, filename_extension);
    }
    std::vector<ShortFileNameEntry> entries{};

    ShortFileNameEntry terminal{};
    memset(terminal.FileName, ' ', sizeof(terminal.FileName));
    for (uint i = 0; i < std::min(filename_name.size(), usz(8)); ++i)
        terminal.FileName[i] = filename_name[i];

    for (uint i = 8; i < std::min(filename_extension.size() + 8, usz(3 + 8)); ++i)
        terminal.FileName[i] = filename_extension[i - 8];

    // toupper
    for (uint i = 0; i < 11; ++i) {
        if (terminal.FileName[i] >= 'a' and terminal.FileName[i] <= 'z')
            terminal.FileName[i] -= 'a' - 'A';
    }

    if (ftype == FileType::Directory)
        terminal.directory(true);
    else
        terminal.archive(true);

    if (not fits_in_short_filename) {
        uint tail_num = 1;
        while (true) {
            auto tail_str = std::format("~{}", tail_num);
            if (tail_str.size() > 7) tail_str.erase(7);
            memcpy(
                &terminal.FileName[8 - tail_str.size()],
                tail_str.data(),
                tail_str.size());

            bool collision = false;
            auto directory_entry_iterator = FAT::DirectoryEntryIterator(
                *this,
                {FAT.data(), FAT.size()},
                parent_cluster);
            for (const auto& entry : directory_entry_iterator) {
                if ((not entry.free_to_use())
                    and (not entry.deleted())
                    and memcmp(&entry.FileName[0], &terminal.FileName[0], 11) == 0) {
                    collision = true;
                    break;
                }
            }
            if (not collision) break;
            ++tail_num;
        }

        u8 checksum = 0;
        for (int i = 0; i < 11; ++i) {
            checksum = ((checksum & 1) ? 0x80 : 0) | (checksum >> 1);
            checksum += terminal.FileName[i];
        }
        DBGMSG("  checksum:{:#x}\n", checksum);

        // >>= LONG FILE NAME ENTRIES <<=
        std::vector<std::string_view> lfn_chunks{};
        usz byte_idx = 0;
        while (byte_idx < filename.size()) {
            usz chunk_start = byte_idx;
            for (int chars = 0; chars < 13 and byte_idx < filename.size(); ++chars) {
                u8 b = filename[byte_idx];
                if ((b & 0x80) == 0)
                    byte_idx += 1;
                else if ((b & 0xe0) == 0xc0)
                    byte_idx += 2;
                else if ((b & 0xf0) == 0xe0)
                    byte_idx += 3;
                else if ((b & 0xf8) == 0xf0)
                    byte_idx += 4;
                // Fallback malformed handling
                else
                    byte_idx += 1;
            }
            lfn_chunks.emplace_back(
                filename.substr(chunk_start, byte_idx - chunk_start));
        }

        usz total_lfn_entries = lfn_chunks.size();
        for (ssz i = total_lfn_entries - 1; i >= 0; --i) {
            LongFileNameEntry lfn{};
            lfn.from_utf8(lfn_chunks[i]);
            lfn.Checksum = checksum;

            // Sequence number allocation. Last written LFN entry gets bit 6 marked high
            u8 order = u8(i + 1);
            if (usz(i) == total_lfn_entries - 1)
                order |= 0b1000000;

            lfn.Order = order;

            entries.emplace_back(*(ShortFileNameEntry*)&lfn);
        }
    }

    DBGMSG("  sfn: \"{}\"\n", std::string_view((const char*)terminal.FileName, 11));

    entries.emplace_back(terminal);

    // 3. Find span of free entries.
    usz index{0};
    usz run_begin_index{0};
    usz run{0};
    auto directory_entry_iterator = FAT::DirectoryEntryIterator(
        *this,
        {FAT.data(), FAT.size()},
        parent_cluster);
    for (auto e : directory_entry_iterator) {
        if (not(e.free_to_use() or e.deleted()))
            run = 0;
        else {
            if (run == 0) run_begin_index = index;
            ++run;
        }
        if (run >= entries.size())
            break;
        ++index;
    }

    DBGMSG(
        "  run of {} directory entries found at index {}\n",
        run,
        run_begin_index);

    // TODO:
    //   3a. If no span found, extend the directory by an extra cluster and
    //       allocate it there.
    if (run < entries.size()) {
        std::print("[FAT]: TODO: Increase directory size while creating file\n");
        return {};
    }

    // 4. Populate span of free entries with pre-calculated entries.

    // Get cluster index within directory where this run of entries is stored.
    const usz entries_per_cluster = (cluster_size() / sizeof(ShortFileNameEntry));
    const usz cluster_index = run_begin_index / entries_per_cluster;
    // NOTE: Points to first LFN, not terminal SFN entry.
    const usz run_begin_index_within_cluster = run_begin_index % entries_per_cluster;
    // NOTE: Points to terminal SFN entry, not first LFN.
    const usz directory_entry_index_offset_within_cluster = (run_begin_index + run - 1) % entries_per_cluster;

    // Use a cluster iterator to navigate that many clusters into the directory.
    auto directory_cluster_iterator = FAT::ClusterIterator(
        {FAT.data(), FAT.size()},
        parent_cluster);
    for (usz i = 0; i < cluster_index; ++i)
        ++directory_cluster_iterator;
    const auto directory_cluster_index = *directory_cluster_iterator;

    auto file = make(ftype,
                     std::string(raw_path),
                     fsd(This.lock()),
                     0,
                     0,
                     parent_cluster,
                     directory_cluster_index,
                     directory_entry_index_offset_within_cluster);

    // Write directory_cluster
    // TODO: It's possible the run spans multiple clusters, if the run starts
    // near the end of a cluster or is really long.
    Device->write(
        file.get(),
        BR.cluster_to_sector(directory_cluster_index) * sector_size()
            + run_begin_index_within_cluster * sizeof(ShortFileNameEntry),
        entries.size() * sizeof(ShortFileNameEntry),
        entries.data(),
        0);

    return file;
}

void FileAllocationTableDriver::close(FileMetadata* file) {
    if (file and file->driver_data())
        delete (FATFileData*)file->driver_data();

    Device->close(file);
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

    auto* data = (FATFileData*)file->driver_data();
    const auto file_first_cluster = data->first_cluster;

    // file empty on disk
    if (file_first_cluster == 0)
        return 0;

    auto traversal = FAT::FileTraversal(*this, file_first_cluster);
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

ssz FileAllocationTableDriver::write(FileMetadata* file, usz offset, usz size, void* buffer, usz flags) {
    auto FAT = std::vector<uint8_t>(fat_byte_count());
    fresh_fat(FAT);
    auto FATspan = std::span(FAT.data(), FAT.size());

    auto* data = (FATFileData*)file->driver_data();

    if (offset + size > file->file_size()) {
        DBGMSG(
            "[FAT]::write(): expanding file \"{}\" to {} bytes (was {})\n",
            file->name(),
            offset + size,

            file->file_size());
        usz last_cluster = 0;
        // If first_cluster is non-zero, that means the file already has clusters
        // allocated; we are expanding an existing file.
        // Often, in FAT, empty files will not have clusters allocated to them.
        if (data->first_cluster) {
            auto last_cluster_iterator = FAT::ClusterIterator(
                FATspan,
                data->first_cluster);
            while (last_cluster_iterator != last_cluster_iterator.end()) {
                last_cluster = *last_cluster_iterator;
                ++last_cluster_iterator;
            }
        }

        const usz increase_amount_bytes = (offset + size) - file->file_size();
        const usz new_file_size = file->file_size() + increase_amount_bytes;
        // - cluster size of 512 bytes
        // - file size of 500 bytes
        // -> write of 4 bytes does not need new clusters!
        const usz remaining_bytes_in_cluster = file->file_size()
                                                   ? (cluster_size() - (file->file_size() % cluster_size()))
                                                   : 0;
        if (increase_amount_bytes > remaining_bytes_in_cluster) {
            // Attempt to allocate new clusters for file
            const usz increase_amount_bytes_in_new_clusters
                = cluster_size() - (increase_amount_bytes % cluster_size());
            const usz increase_amount_clusters
                = (increase_amount_bytes_in_new_clusters + cluster_size() - 1)
                  / cluster_size();
            DBGMSG("[FAT]: increase file {} by {} clusters\n", file->name(), increase_amount_clusters);

            std::vector<usz> free_cluster_indices{};

            // TODO: FAT variant handling; this is FAT32 only
            auto* FATdata = (uint32_t*)FAT.data();
            for (usz i = 0; i < cluster_count(); ++i) {
                auto entry = FATdata[i] & 0x0fffffff;
                if (entry == 0) {
                    DBGMSG("[FAT]: cluster {} is free\n", i);
                    free_cluster_indices.emplace_back(i);
                }
                // Once we have found enough clusters to increase the file size by the
                // requested amount, we can stop looking.
                if (free_cluster_indices.size() >= increase_amount_clusters)
                    break;
            }
            if (free_cluster_indices.size() < increase_amount_clusters) {
                std::print(
                    "[FAT]: not enough space to increase size of file \"{}\"\n"
                    "  needed {} clusters, found {} free\n",
                    file->name(),
                    increase_amount_clusters,
                    free_cluster_indices.size());
                return -1;
            }
            for (usz i = 0; i < free_cluster_indices.size() - 1; ++i) {
                DBGMSG(
                    "  pointing cluster {} to cluster {}\n",
                    free_cluster_indices.at(i),
                    free_cluster_indices.at(i + 1));
                FATdata[free_cluster_indices.at(i)] = free_cluster_indices.at(i + 1);
            }
            // End Of File Cluster Chain Marker
            DBGMSG(
                "  writing end-of-file marker to cluster {}\n",
                free_cluster_indices.back());
            FATdata[free_cluster_indices.back()] = 0x0fffffff;

            // Over-write old end-of-file cluster chain marker with first "free"
            // cluster. We only do this if the file actually had clusters allocated to
            // it previously. Linked list analogy: setting last->next only iff last.
            if (last_cluster) {
                DBGMSG(
                    "  overwriting old end-of-file marker at cluster {} to cluster {}\n",
                    last_cluster,
                    free_cluster_indices.front());
                FATdata[last_cluster] = free_cluster_indices.front();
            }
            else {
                DBGMSG("  first {} clusters allocated for file\n", free_cluster_indices.size());
                data->first_cluster = free_cluster_indices.front();
            }

            // Write FAT(s) back to disk
            const usz FAToffset = first_fat_sector() * sector_size();
            for (uint i = 0; i < BR.BPB.NumFATsPresent; ++i) {
                for (auto c : free_cluster_indices) {
                    DBGMSG(
                        "  writing previously-free cluster entry {} with value {} in FAT {}\n",
                        c,
                        FATdata[c],
                        i);
                    Device->write(
                        file,
                        FAToffset
                            + c * sizeof(*FATdata)
                            + i * fat_byte_count(),
                        sizeof(*FATdata),
                        &FATdata[c],
                        0);
                }

                if (last_cluster) {
                    DBGMSG(
                        "  writing previous end-of-chain cluster entry {} with value {} in FAT {}\n",
                        last_cluster,
                        FATdata[last_cluster],
                        i);
                    Device->write(
                        file,
                        FAToffset
                            + last_cluster * sizeof(*FATdata)
                            + i * fat_byte_count(),
                        sizeof(*FATdata),
                        &FATdata[last_cluster],
                        0);
                }
            }

            // Update directory entry of file with new file size
            DBGMSG("  directory entry within cluster {}\n", data->parent_entry_cluster);
            const auto entry_sector = BR.cluster_to_sector(data->parent_entry_cluster);
            DBGMSG(
                "  cluster {} at sector {} AKA byte offset {}\n",
                data->parent_entry_cluster,
                entry_sector,
                entry_sector * sector_size());
            DBGMSG("  entry index {}\n", data->entry_index_within_cluster);
            const auto entry_offset = entry_sector * sector_size()
                                      + data->entry_index_within_cluster * sizeof(ShortFileNameEntry);
            DBGMSG(
                "  writing new file size {}, cluster {} at entry at offset {}\n",
                new_file_size,
                data->first_cluster,
                entry_offset);
            ShortFileNameEntry entry{};
            Device->read(
                file,
                entry_offset,
                sizeof(ShortFileNameEntry),
                &entry,
                0);

            entry.ClusterNumberL = data->first_cluster;
            entry.ClusterNumberH = data->first_cluster >> 16;
            entry.FileSizeInBytes = new_file_size;
            // TODO: update modified time

            Device->write(
                file,
                entry_offset,
                sizeof(ShortFileNameEntry),
                &entry,
                0);
        }
    }

    // Use a ClusterIterator to follow the file's data around the disk.
    auto iterator = FAT::ClusterIterator(
        FATspan,
        data->first_cluster);

    const usz before_clusters = offset / cluster_size();
    usz offset_within_cluster = offset % cluster_size();
    for (usz i = 0; i < before_clusters and iterator != iterator.end(); ++i)
        ++iterator;

    if (iterator == iterator.end()) {
        std::print("[FAT]: Write to file beyond cluster extents... resize went wrong?\n");
        return -1;
    }

    auto cluster_data = std::vector<u8>(cluster_size());

    usz bytes_written{0};
    while (bytes_written < size and iterator != iterator.end()) {
        const auto cluster_index = *iterator;
        const auto cluster_sector = BR.cluster_to_sector(cluster_index);
        const auto cluster_byte_offset = cluster_sector * sector_size();

        const usz available_in_cluster = cluster_size() - offset_within_cluster;
        const usz remaining_to_write = size - bytes_written;
        const usz chunk_size = std::min(available_in_cluster, remaining_to_write);

        // Only read cluster if we aren't overwriting entire cluster.
        if (chunk_size != cluster_size())
            read_cluster_into(cluster_data, cluster_index);

        DBGMSG("  writing data into cluster {} at offset {}\n", cluster_index, offset_within_cluster);
        memcpy(
            cluster_data.data() + offset_within_cluster,
            ((u8*)buffer) + bytes_written,
            chunk_size);

        DBGMSG("  writing {} bytes into cluster {} (offset {})\n", chunk_size, cluster_index, cluster_byte_offset);
        if (Device->write(
                file,
                cluster_byte_offset,
                cluster_size(),
                cluster_data.data(),
                flags)
            != (ssz)cluster_size())
            return -1;

        bytes_written += chunk_size;
        offset_within_cluster = 0;
        ++iterator;
    }

    return bytes_written;
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
