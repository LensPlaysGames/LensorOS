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

#include <format>

#include <storage/filesystem_drivers/file_allocation_table.h>

#include <debug.h>
#include <fat_definitions.h>
#include <storage/file_metadata.h>
#include <integers.h>
#include <linked_list.h>
#include <smart_pointer.h>
#include <string.h>

// Uncomment the following directive for extra debug information output.
//#define DEBUG_FAT

#ifdef DEBUG_FAT
#   define DBGMSG(...) std::print(__VA_ARGS__)
#else
#   define DBGMSG(...)
#endif

void FileAllocationTableDriver::print_fat(BootRecord* br) {
    if (br == nullptr)
        return;

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
               , br->total_clusters()
               , br->BPB.NumSectorsPerCluster
               , br->BPB.total_sectors()
               , u16(br->BPB.NumBytesPerSector)
               , br->fat_sectors()
               , br->BPB.first_fat_sector()
               , br->first_data_sector()
               , br->first_root_directory_sector()
               );
}

FATType fat_type(BootRecord* br) {
    u64 totalClusters = br->total_clusters();
    if (totalClusters == 0)
        return FATType::ExFAT;
    else if (totalClusters < 4085)
        return FATType::FAT12;
    else if (totalClusters < 65525)
        return FATType::FAT16;
    else return FATType::FAT32;
}

bool FileAllocationTableDriver::test(StorageDeviceDriver* driver) {
    if (driver == nullptr)
        return false;

    auto buffer = SmartPtr<u8[]>(new u8[512], 512);
    if (buffer.get() == nullptr)
        return false;

    bool out { false };
    driver->read(0, 512, buffer.get());
    auto* br = reinterpret_cast<BootRecord*>(buffer.get());
    u64 totalSectors = br->BPB.TotalSectors16 == 0
        ? br->BPB.TotalSectors32 : br->BPB.TotalSectors16;
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
    out = (br->Magic == 0xaa55
           && totalSectors != 0
           && br->BPB.NumBytesPerSector >= 512
           && br->BPB.NumBytesPerSector <= 4096
           && (br->BPB.NumBytesPerSector
               & (br->BPB.NumBytesPerSector - 1)) == 0
           && (br->BPB.NumSectorsPerCluster
               & (br->BPB.NumSectorsPerCluster - 1)) == 0
           && br->BPB.NumFATsPresent > 0);

#ifdef DEBUG_FAT
    print_fat(br);
#endif /* DEBUG_FAT */

    return out;
}

FileMetadata FileAllocationTableDriver::file(StorageDeviceDriver* driver, const String& givenPath) {
    if (driver == nullptr) {
        return {};
    }
    u64 givenPathLength = givenPath.length();
    if (givenPathLength <= 1) {
        return {};
    }
    // Translate path (FAT has very limited file names).
    String path = String((const char*)&givenPath.bytes()[1]);
    for (u8 i = path.length(); i < 11; ++i)
        path += " ";

    u8* currentCharacter = path.bytes();
    for (u64 i = 0; i < path.length(); ++i) {
        if (*currentCharacter >= 97 && *currentCharacter <= 122)
            *currentCharacter -= 32;
        else if (*currentCharacter == '.')
            *currentCharacter = ' ';

        currentCharacter++;
    }
    DBGMSG("[FAT]: Looking for file at {}\n"
           "  Translated path: {}\n"
           , std::string_view{givenPath.data(), givenPath.length()}
           , std::string_view{path.data(), path.length()}
           );
    // TODO: Take in cached Boot Record from filesystem.
    SmartPtr<u8[]> sector(new u8[sizeof(BootRecord)], sizeof(BootRecord));
    if (sector.get() == nullptr) {
        return {};
    }
    driver->read(0, sizeof(BootRecord), sector.get());
    auto* br = reinterpret_cast<BootRecord*>(sector.get());
    FATType type = fat_type(br);

    // TODO: Take in cached FAT from filesystem.
    SmartPtr<u8[]> FAT(new u8[br->BPB.NumBytesPerSector], br->BPB.NumBytesPerSector);
    u64 lastFATsector { 0 };

    constexpr u64 lfnBufferSize = 27;
    SmartPtr<u8[]> lfnBuffer(new u8[lfnBufferSize], lfnBufferSize);
    u32 clusterIndex = br->sector_to_cluster(br->first_root_directory_sector());
    bool moreClusters = { true };
    while (moreClusters) {
        u64 clusterSize = br->BPB.NumSectorsPerCluster * br->BPB.NumBytesPerSector;
        SmartPtr<u8[]> clusterContents(new u8[clusterSize], clusterSize);
        u64 clusterSector = br->cluster_to_sector(clusterIndex);
        driver->read(clusterSector * br->BPB.NumBytesPerSector
                     , clusterSize
                     , clusterContents.get());
        auto* entry = reinterpret_cast<ClusterEntry*>(clusterContents.get());
        bool lfnBufferFull { false };
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
            String fileName(reinterpret_cast<const char*>(&entry->FileName[0]), 11);
            if (lfnBufferFull) {
               fileName += String((const char*)&lfnBuffer[0], lfnBufferSize);
               lfnBufferFull = false;
            }
            String fileType("");
            if (entry->read_only())
                fileType += "read-only ";
            if (entry->hidden())
                fileType += "hidden ";
            if (entry->system())
                fileType += "system ";
            if (entry->archive())
                fileType += "archive ";
            if (entry->directory())
                fileType += "directory ";
            else if (entry->volume_id())
                fileType += "volume identifier ";
            else fileType += "file ";
            DBGMSG("    Found {}named {}\n",
                   std::string_view { fileType.data(), fileType.length() },
                   std::string_view { fileName.data(), fileName.length() });
            if (fileName == path) {
                DBGMSG("  Found file!\n");
                // TODO: directory vs. file metadata
                u64 byteOffset = br->cluster_to_sector(entry->get_cluster_number())
                    * br->BPB.NumBytesPerSector;
                return { fileName, false, driver, this, entry->FileSizeInBytes, byteOffset };
            }
            entry++;
        }
        // Check if this is the last cluster in the chain.
        u64 clusterNumber = entry->get_cluster_number();
        // FIXME: This assumes FAT32, but we should really determine type dynamically.
        u64 FAToffset = 0;
        switch (type) {
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
        u64 FATsector = br->BPB.first_fat_sector() + (FAToffset / br->BPB.NumBytesPerSector);
        u64 entryOffset = FAToffset % br->BPB.NumBytesPerSector;
        if (entryOffset <= 1 || FATsector == lastFATsector)
            break;

        lastFATsector = FATsector;
        driver->read(FATsector * br->BPB.NumBytesPerSector
                     , br->fat_sectors() * br->BPB.NumBytesPerSector
                     , FAT.get());
        u64 tableValue = 0;
        switch (type) {
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
