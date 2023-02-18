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
#include <format>
#include <integers.h>
#include <storage/file_metadata.h>
#include <storage/filesystem_drivers/file_allocation_table.h>
#include <vector>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_FAT

#ifdef DEBUG_FAT
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...)
#endif

/// Given "/foo/bar/baz.txt" return "foo" and set path to "bar/baz.txt"
/// Given "/" return "/"
std::string pop_filename_from_front_of_path(std::string& raw_path) {
    /// Strip leading slash.
    std::string path = raw_path;
    if (path.starts_with("/")) path = path.substr(1);
    if (path.size() < 1) return raw_path;

    raw_path = path;
    size_t first_sep = path.find_first_of("/");
    if (first_sep == std::string::npos) return path;

    std::string out = path.substr(0, first_sep);
    raw_path = path.substr(first_sep);
    return out;
}

void FileAllocationTableDriver::print_fat(BootRecord& br) {
    std::print("File Allocation Table Boot Record:\n"
               "  Total Clusters:      {}\n"
               "  Sectors / Cluster:   {}\n"
               "  Total Sectors:       {}\n"
               "  Bytes / Sector:      {}\n"
               "  Sectors / FAT:       {}\n"
               "  Sector Offsets:\n"
               "    FATs:      {}\n"
               "    Data:      {}\n"
               "    Root Dir.: {}\n"
               "\n"
               , br.total_clusters()
               , br.BPB.NumSectorsPerCluster
               , br.BPB.total_sectors()
               , u16(br.BPB.NumBytesPerSector)
               , br.fat_sectors()
               , br.BPB.first_fat_sector()
               , br.first_data_sector()
               , br.first_root_directory_sector()
               );
}

FATType FileAllocationTableDriver::fat_type(BootRecord& br) {
    u64 totalClusters = br.total_clusters();
    if (totalClusters == 0) return FATType::ExFAT;
    else if (totalClusters < 4085) return FATType::FAT12;
    else if (totalClusters < 65525) return FATType::FAT16;
    else return FATType::FAT32;
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
                && (br.BPB.NumBytesPerSector
                    & (br.BPB.NumBytesPerSector - 1)) == 0
                && (br.BPB.NumSectorsPerCluster
                    & (br.BPB.NumSectorsPerCluster - 1)) == 0
                && br.BPB.NumFATsPresent > 0);

    if (!out) return nullptr;

#ifdef DEBUG_FAT
    print_fat(br);
#endif /* DEBUG_FAT */

    auto fs = std::make_shared<FileAllocationTableDriver>(std::move(driver), std::move(br));
    fs->This = fs;
    return std::static_pointer_cast<FilesystemDriver>(fs);
}

auto FileAllocationTableDriver::translate_filename(std::string_view raw_filename) -> std::string {
    std::string path = raw_filename;

    // Pad filename to 11 bytes long (Length of all FAT filenames without LFN entry)
    for (usz i = raw_filename.size(); i < 11; ++i) { path += " "; }

    // Alter filename to have legal characters.
    for (usz i = 0; i < raw_filename.size(); ++i) {
        if (path[i] >= 97 && path[i] <= 122) path[i] -= 32; // toupper
        else if (path[i] == '.') path[i] = ' ';
    }

    DBGMSG("[FAT]: Translating filename \"{}\"; Got \"{}\"\n", raw_filename , path);

    return path;
}

auto FileAllocationTableDriver::open(std::string_view raw_path) -> std::shared_ptr<FileMetadata> {
    DBGMSG("[FAT]: Attempting to open file {}\n", raw_path);
    if (Device == nullptr) {
        /// Should never get here.
        std::print("FileAllocationTableDriver::open(): Device is null!\n");
        return {};
    }

#ifdef DEBUG_FAT
    auto __this = This.lock();
    if (!__this) {
        /// Should never get here.
        std::print("FileAllocationTableDriver::open(): This is null!\n");
        return {};
    }
#endif

    /// Strip leading slash.
    if (raw_path.starts_with("/")) raw_path = raw_path.substr(1);
    if (raw_path.size() < 1) {
        DBGMSG("FileAllocationTableDriver::open(): Invalid path: {}\n", raw_path);
        return {};
    }

    // Get first filename from path.
    // Given path "foo/bar/bas.exe", return "foo" as a legal FAT
    // filename, and alter given path to be "past" that + directory
    // separator.

    // Translate path (FAT has very limited file names).
    auto path = translate_filename(raw_path);

    // TODO: Take in cached FAT from filesystem.
    std::vector<u8> FAT(BR.BPB.NumBytesPerSector);
    u64 lastFATsector { 0 };

    // TODO: ExFAT will need it's own code flow, essentially.

    constexpr u64 lfnBufferSize = 27;
    const u64 clusterSize = BR.BPB.NumSectorsPerCluster * BR.BPB.NumBytesPerSector;
    u8 lfnBuffer[lfnBufferSize];
    u32 clusterIndex = BR.sector_to_cluster(BR.first_root_directory_sector());
    bool moreClusters = true;
    while (moreClusters) {
        std::vector<u8> clusterContents(clusterSize);
        u64 clusterSector = BR.cluster_to_sector(clusterIndex);
        Device->read_raw(clusterSector * BR.BPB.NumBytesPerSector
                         , clusterSize
                         , clusterContents.data());
        auto* entry = reinterpret_cast<ClusterEntry*>(clusterContents.data());
        bool lfnBufferFull{};
        while (entry->FileName[0] != 0) {
            if (entry->FileName[0] == 0xe5)
                continue;

            if (entry->long_file_name()) {
                auto* lfn = reinterpret_cast<LFNClusterEntry*>(entry);
                u8 offset = 0;
                memcpy(&lfnBuffer[offset], &lfn->Characters1[0], sizeof(u16) * 5);
                offset += 5;
                memcpy(&lfnBuffer[offset], &lfn->Characters1[0], sizeof(u16) * 6);
                offset += 6;
                memcpy(&lfnBuffer[offset], &lfn->Characters3[0], sizeof(u16) * 2);
                lfnBufferFull = true;
                entry++;
                continue;
            }

            std::string fileName(reinterpret_cast<const char*>(&entry->FileName[0]), 11);
            if (lfnBufferFull) {
               fileName.append((const char*)lfnBuffer, lfnBufferSize);
               lfnBufferFull = false;
            }
            std::string fileType;

            if (entry->read_only()) fileType += "read-only ";
            if (entry->hidden()) fileType += "hidden ";
            if (entry->system()) fileType += "system ";
            if (entry->archive()) fileType += "archive ";

            if (entry->directory()) fileType += "directory ";
            else if (entry->volume_id()) fileType += "volume identifier ";
            else fileType += "file ";

            DBGMSG("    Found {}named {}\n", fileType , fileName);

            if (fileName == path) {
                DBGMSG("  Found file!\n");
                // TODO: directory vs. file metadata
                u64 byteOffset = BR.cluster_to_sector(entry->get_cluster_number())
                    * BR.BPB.NumBytesPerSector;
                return std::make_shared<FileMetadata> (
                    std::move(fileName),
                    std::static_pointer_cast<StorageDeviceDriver>(This.lock()),
                    u32(entry->FileSizeInBytes),
                    (void*) byteOffset
                );
            }

            entry++;
        }
        // Check if this is the last cluster in the chain.
        u64 clusterNumber = entry->get_cluster_number();
        u64 FAToffset = 0;
        switch (Type) {
        case FATType::FAT12:
            FAToffset = clusterNumber + (clusterNumber / 2);
            break;
        case FATType::FAT16:
            FAToffset = clusterNumber * 2;
            break;
        case FATType::ExFAT:
        case FATType::FAT32:
        default:
            FAToffset = clusterNumber * 4;
            break;
        }
        u64 FATsector = BR.BPB.first_fat_sector() + (FAToffset / BR.BPB.NumBytesPerSector);
        u64 entryOffset = FAToffset % BR.BPB.NumBytesPerSector;
        if (entryOffset <= 1 || FATsector == lastFATsector)
            break;

        lastFATsector = FATsector;
        Device->read_raw(FATsector * BR.BPB.NumBytesPerSector
                         , BR.fat_sectors() * BR.BPB.NumBytesPerSector
                         , FAT.data());
        u64 tableValue = 0;
        switch (Type) {
        case FATType::FAT12:
            tableValue = *(reinterpret_cast<u16*>(&FAT[entryOffset]));
            if (clusterNumber & 0b1)
                tableValue >>= 4;
            else tableValue &= 0x0fff;
            if (tableValue >= 0x0ff8)
                moreClusters = false;
            // TODO: Hande tableValue == 0x0ff7 (bad cluster)
            break;
        case FATType::FAT16:
            tableValue = *(reinterpret_cast<u16*>(&FAT[entryOffset]));
            tableValue &= 0xffff;
            if (tableValue >= 0xfff8)
                moreClusters = false;
            // TODO: Hande tableValue == 0xfff7 (bad cluster)
            break;
        case FATType::FAT32:
            tableValue = *(reinterpret_cast<u32*>(&FAT[entryOffset]));
            tableValue &= 0xfffffff;
            if (tableValue >= 0x0ffffff8)
                moreClusters = false;
            // TODO: Hande tableValue == 0x0ffffff7 (bad cluster)
            break;
        case FATType::ExFAT:
            tableValue = *(reinterpret_cast<u32*>(&FAT[entryOffset]));
            break;
        default:
            break;
        }

        clusterIndex = tableValue;
    }
    return {};
}
