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

#ifndef LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H
#define LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H

#include <fat_definitions.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>
#include <string>
#include <vector>

class FileAllocationTableDriver final : public FilesystemDriver {
    /// This constructor is only used internally in try_create() and is always
    /// invoked via std::make_shared().
    explicit FileAllocationTableDriver(std::shared_ptr<StorageDeviceDriver>&& device, BootRecord&& br)
        : Device(std::move(device))
        , BR(std::move(br))
        , Type(fat_type(BR)) {}

    /// Weak reference to ourselves. Every FileAllocationTableDriver is created
    /// as a shared_ptr. Upon creation, this is set to a weak_ptr to that
    /// shared_ptr.
    ///
    /// We do this because any files we open need to receive a copy of the
    /// shared_ptr, so we need to store it *somewhere*. We can’t store it as
    /// a shared_ptr since that would mean that it would never be destroyed.
    ///
    /// Instead, we can just use a weak_ptr here. This works because the lifetime
    /// of the shared_ptr control block is always longer than the lifetime of this
    /// object, and if we’re still alive, then so is the shared_ptr.
    ///
    /// When the last shared_ptr to us is destroyed, the destructor of the
    /// shared_ptr will invoke our destructor, which will destroy this weak_ptr.
    std::weak_ptr<FileAllocationTableDriver> This{};

    /// Strong reference to the underlying storage device.
    std::shared_ptr<StorageDeviceDriver> Device{};

    /// FAT boot record.
    BootRecord BR{};

    /// FAT type.
    FATType Type{};

    friend std::shared_ptr<FileAllocationTableDriver>
        std::make_shared(std::shared_ptr<StorageDeviceDriver>&& device, BootRecord&& br);

    static auto fat_type(BootRecord& br) -> FATType;

    /// This is so we have something that we can call begin() and end() on because
    /// calling begin()/end() on the driver itself would be a bit weird semantically.
    struct DirIteratorHelper {
        FileAllocationTableDriver& Driver;
        u32 ClusterIndex = Driver.BR.sector_to_cluster(Driver.BR.first_root_directory_sector());

        /// This does the actual iterating.
        struct Iterator {
            FileAllocationTableDriver& Driver;
            u32 ClusterIndex;

            /// Constants.
            const u64 ClusterSize = Driver.BR.BPB.NumSectorsPerCluster * Driver.BR.BPB.NumBytesPerSector;

            /// Iteration data.
            std::vector<u8> ClusterContents{ClusterSize};
            u64 LastFATSector = 0;
            bool MoreClusters = true;
            bool ClearLFN = false;

            /// The current entry.
            struct EntryType {
                ClusterEntry* CE{};
                u64 ByteOffset{};
                std::string FileName;
                std::string LongFileName;
            } Entry{};

            explicit Iterator(FileAllocationTableDriver& driver, u32 directoryCluster);
            auto operator++() -> Iterator&;
            auto operator*() -> EntryType& { return Entry; }
            auto operator->() -> EntryType* { return &Entry; }
            bool operator!=(std::default_sentinel_t) const { return MoreClusters; }

        private:
            /// Read the next cluster unconditionally.
            void ReadNextCluster();

            /// Read the next cluster if there is one. If there isn’t, set MoreClusters to false.
            void TryReadNextCluster();
        };

        DirIteratorHelper(FileAllocationTableDriver& driver) : Driver(driver) {}
        DirIteratorHelper(FileAllocationTableDriver& driver, u32 directoryCluster)
        : Driver(driver), ClusterIndex(directoryCluster) {}

        /// Get an iterator that points to the first entry.
        auto begin() -> Iterator { return Iterator{Driver, ClusterIndex}; }

        /// We don’t really have a predetermined ‘end’, so this just returns a dummy value.
        auto end() -> std::default_sentinel_t { return {}; }
    };

    auto for_each_dir_entry() -> DirIteratorHelper { return DirIteratorHelper{*this}; }
    auto for_each_dir_entry_in(u32 directoryCluster) -> DirIteratorHelper { return DirIteratorHelper{*this, directoryCluster}; }

    /// NOTE: If directoryCluster == -1 (default), it will be replaced
    /// with the directory cluster of the root directory.
    std::shared_ptr<FileMetadata> traverse_path(std::string_view raw_path, u32 directoryCluster = -1);

public:
    static void print_fat(BootRecord&);

    auto open(std::string_view path) -> std::shared_ptr<FileMetadata> final;
    void close(FileMetadata* file) final { Device->close(file); }

    ssz read(FileMetadata* file, usz offs, usz size, void* buffer) final {
        return Device->read_raw(usz(file->driver_data()) + offs, size, buffer);
    }

    ssz read_raw(usz offs, usz bytes, void* buffer) final {
        return Device->read_raw(offs, bytes, buffer);
    }

    ssz write(FileMetadata* file, usz offset, usz size, void* buffer) final {
        // TODO: Fail? if this would increase file size. I feel like we
        // don't want to write past the end of the file, just in case
        // there is stuff there, right? So we will have to figure out
        // how to make a file bigger in FAT.
        return Device->write(file, usz(file->driver_data()) + offset, size, buffer);
    }

    const char* name() final { return "File Allocation Table"; }

    auto device() -> std::shared_ptr<StorageDeviceDriver> final { return Device; }

    /// Try to create a FileAllocationTableDriver from the given storage device.
    static auto try_create(std::shared_ptr<StorageDeviceDriver>) -> std::shared_ptr<FilesystemDriver>;

    static auto translate_filename(std::string_view path) -> std::string;
};

#endif /* LENSOR_OS_FILE_ALLOCATION_TABLE_DRIVER_H */
