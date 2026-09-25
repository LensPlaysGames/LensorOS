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

#ifndef LENSOROS_KERNEL_FILE_ALLOCATION_TABLE_DIRECTORY_ITERATOR_H
#define LENSOROS_KERNEL_FILE_ALLOCATION_TABLE_DIRECTORY_ITERATOR_H

#include <fat_cluster_iterator.h>
#include <fat_definitions.h>
#include <fat_file_data_iterator.h>
#include <lensor/files.h>
#include <storage/filesystem_drivers/file_allocation_table.h>

#include <array>
#include <iterator>
#include <print>
#include <utility>
#include <vector>

namespace FAT {

struct DirectoryEntryIterator {
    using iterator_category = std::forward_iterator_tag;
    using value_type = ShortFileNameEntry;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type&;

    explicit DirectoryEntryIterator(
        FileAllocationTableDriver& driver,
        std::span<uint8_t> FAT,
        u64 firstClusterIndex)
        : data_iterator(driver, FAT, firstClusterIndex) {}

    explicit DirectoryEntryIterator(
        FileAllocationTableDriver& driver,
        ClusterIterator&& in_iterator)
        : data_iterator(driver, std::move(in_iterator)) {}

    explicit DirectoryEntryIterator(FileDataIterator&& in_iterator)
        : data_iterator(std::move(in_iterator)) {}

    auto operator++() -> DirectoryEntryIterator& {
        ++current_directory_entry_index;

        const auto entries_per_cluster = data_iterator.get_driver().cluster_size()
                                         / sizeof(ShortFileNameEntry);
        if (current_directory_entry_index >= entries_per_cluster)
            next_cluster();

        return *this;
    }
    auto operator++(int) -> DirectoryEntryIterator {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
    reference operator*() {
        return *current_directory_entry();
    }

    struct Proxy {
        value_type value;
        value_type* operator->() { return &value; }
    };
    Proxy operator->() {
        return Proxy{**this};
    }

    bool operator!=(std::default_sentinel_t) const {
        const auto entries_per_cluster = data_iterator.get_driver().cluster_size()
                                         / sizeof(ShortFileNameEntry);
        return data_iterator != data_iterator.end()
               and current_directory_entry_index < entries_per_cluster;
    };
    bool operator==(std::default_sentinel_t v) const {
        return not operator!=(v);
    };

    // Get an iterator that points to the first entry.
    auto begin() -> DirectoryEntryIterator {
        return DirectoryEntryIterator{data_iterator.begin()};
    }

    // We don’t really have a predetermined ‘end’, so this just returns a
    // dummy value.
    auto end() const -> std::default_sentinel_t { return {}; }

    auto cluster_index() const { return current_cluster_index; }
    auto entry_index() const { return current_directory_entry_index; }

   private:
    void next_cluster() {
        ++current_cluster_index;
        current_directory_entry_index = 0;
        ++data_iterator;
    }

    ShortFileNameEntry* current_directory_entry() {
        return (ShortFileNameEntry*)((*data_iterator).data()
                                     + current_directory_entry_index * sizeof(ShortFileNameEntry));
    }

    FileDataIterator data_iterator;
    usz current_cluster_index{0};
    usz current_directory_entry_index{0};
};

struct DirectoryEntry {
    std::array<uint8_t, 11> short_name{};
    std::string name{};

    // How many clusters full of directory entries come before the cluster
    // containing this directory entry, within the containing directory.
    // This directory entry is within cluster `directory_clusters[within_cluster]`.
    u32 within_cluster{};
    // Index of directory entry within the cluster it is located in.
    u32 entry_index{};

    u32 cluster_number{};
    u32 file_size_in_bytes{};

    u8 creation_time_tenth_of_seconds{};
    u8 creation_time_seconds{};
    u8 creation_time_minutes{};
    u8 creation_time_hours{};
    u8 creation_time_day{};
    u8 creation_time_month{};
    u8 creation_time_year{};

    u8 access_time_day{};
    u8 access_time_month{};
    u8 access_time_year{};

    u8 modification_time_seconds{};
    u8 modification_time_minutes{};
    u8 modification_time_hours{};
    u8 modification_time_day{};
    u8 modification_time_month{};
    u8 modification_time_year{};

    bool read_only{};
    bool hidden{};
    bool system{};
    bool volume_id{};
    bool directory{};
    bool archive{};
};

struct DirectoryIterator {
    using iterator_category = std::input_iterator_tag;
    using value_type = DirectoryEntry;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type*;
    using reference = value_type;

    explicit DirectoryIterator(
        FileAllocationTableDriver& driver,
        std::span<uint8_t> FAT,
        u64 firstClusterIndex)
        : directory_entry_iterator(driver, FAT, firstClusterIndex) { consume(); }

    explicit DirectoryIterator(
        FileAllocationTableDriver& driver,
        ClusterIterator&& in_iterator)
        : directory_entry_iterator(driver, std::move(in_iterator)) { consume(); }

    explicit DirectoryIterator(FileDataIterator&& in_iterator)
        : directory_entry_iterator(std::move(in_iterator)) { consume(); }

    explicit DirectoryIterator(DirectoryEntryIterator&& in_iterator)
        : directory_entry_iterator(std::move(in_iterator)) { consume(); }

    auto operator++() -> DirectoryIterator& {
        consume();
        return *this;
    }
    auto operator++(int) -> DirectoryIterator {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }
    reference operator*() {
        return current_entry;
    }

    struct Proxy {
        value_type value;
        value_type* operator->() { return &value; }
    };
    Proxy operator->() {
        return Proxy{**this};
    }

    bool operator!=(std::default_sentinel_t) const {
        return directory_entry_iterator != directory_entry_iterator.end();
    };
    bool operator==(std::default_sentinel_t v) const {
        return not operator!=(v);
    };

    // Get an iterator that points to the first entry.
    auto begin() -> DirectoryIterator {
        return DirectoryIterator{directory_entry_iterator.begin()};
    }

    // We don’t really have a predetermined ‘end’, so this just returns a
    // dummy value.
    auto end() const -> std::default_sentinel_t { return {}; }

   private:
    // Consume entries that pertain to a single file.
    void consume() {
        while (
            directory_entry_iterator != directory_entry_iterator.end()  // iterator valid
            and (directory_entry_iterator->free_to_use()                // entry free or deleted
                 or directory_entry_iterator->deleted()))
            ++directory_entry_iterator;

        current_entry = DirectoryEntry();
        auto entry = *directory_entry_iterator;

        // Handle Long File Name (LFN) entries
        {
            while (entry.long_file_name()) {
                LongFileNameEntry& lfn_entry = *(LongFileNameEntry*)&entry;

                const auto utf16_lfn = lfn_entry.utf16_data();

                auto utf8_lfn = utf16_to_utf8(
                    std::string_view(
                        (const char*)utf16_lfn.data(),
                        utf16_lfn.size() * (sizeof(*utf16_lfn.data()) / sizeof(const char))));

                // Long file names have some gubbins at the end.
                std::string_view marker("\0\xff", 2);
                if (auto i = utf8_lfn.find_first_of(marker); i != std::string::npos)
                    utf8_lfn.erase(i);

                // NOTE/FIXME: It's possible we are meant to prepend segments, not append
                // them. I don't know :)
                current_entry.name += utf8_lfn;

                entry = *++directory_entry_iterator;
            }
        }

        // One or more Long File Name entries followed by an empty entry... no actual file!
        // FIXME: better way to handle this, most likely
        if (entry.free_to_use() or entry.deleted()) {
            std::print("[FAT]: Long File Name entries NOT followed by actual file entry!!\n");
            return;
        }

        // Handle actual (short file name) entry
        memcpy(
            current_entry.short_name.data(),
            &entry.FileName[0],
            sizeof(entry.FileName));

        current_entry.within_cluster = directory_entry_iterator.cluster_index();
        current_entry.entry_index = directory_entry_iterator.entry_index();

        current_entry.cluster_number = entry.get_cluster_number();
        current_entry.file_size_in_bytes = entry.FileSizeInBytes;

        current_entry.read_only = entry.read_only();
        current_entry.hidden = entry.hidden();
        current_entry.system = entry.system();
        current_entry.volume_id = entry.volume_id();
        current_entry.directory = entry.directory();
        current_entry.archive = entry.archive();

        current_entry.creation_time_tenth_of_seconds = entry.CTimeTenthsSecond;
        current_entry.creation_time_seconds = entry.ctime_second();
        current_entry.creation_time_minutes = entry.ctime_minute();
        current_entry.creation_time_hours = entry.ctime_hour();
        current_entry.creation_time_day = entry.cdate_day();
        current_entry.creation_time_month = entry.cdate_month();
        current_entry.creation_time_year = entry.cdate_year();
        current_entry.access_time_day = entry.adate_day();
        current_entry.access_time_month = entry.adate_month();
        current_entry.access_time_year = entry.adate_year();
        current_entry.modification_time_seconds = entry.mtime_second();
        current_entry.modification_time_minutes = entry.mtime_minute();
        current_entry.modification_time_hours = entry.mtime_hour();
        current_entry.modification_time_day = entry.mdate_day();
        current_entry.modification_time_month = entry.mdate_month();
        current_entry.modification_time_year = entry.mdate_year();

        // Derive from short name, if necessary.
        if (current_entry.name.empty()) {
            for (uint i = 0; i < 8; ++i) {
                if (current_entry.short_name[i] <= ' ')
                    break;
                current_entry.name += current_entry.short_name[i];
            }
            // extension present in short name
            if (current_entry.short_name[8] > ' ') {
                current_entry.name += '.';
                for (uint i = 8; i < 11; ++i) {
                    if (current_entry.short_name[i] <= ' ')
                        break;
                    current_entry.name += current_entry.short_name[i];
                }
            }
        }

        ++directory_entry_iterator;
    }

    DirectoryEntryIterator directory_entry_iterator;
    DirectoryEntry current_entry{};
};

struct FileTraversal {
    explicit FileTraversal(
        FileAllocationTableDriver& driver,
        u64 firstClusterIndex)
        : driver(driver)
        , first_cluster(firstClusterIndex) {
        if (first_cluster == u32(-1))
            first_cluster = driver.root_directory_cluster();

        driver.fresh_fat(FAT);
    }

    FileDataIterator go_file_data() {
        return FileDataIterator(
            driver,
            std::span(FAT.data(), FAT.size()),
            first_cluster);
    }

    DirectoryIterator go_directory() {
        return DirectoryIterator(
            driver,
            std::span(FAT.data(), FAT.size()),
            first_cluster);
    }

   private:
    std::vector<uint8_t> FAT{};
    FileAllocationTableDriver& driver;
    u32 first_cluster{};
};

}  // namespace FAT

#endif /* LENSOROS_KERNEL_FILE_ALLOCATION_TABLE_DIRECTORY_ITERATOR_H */
