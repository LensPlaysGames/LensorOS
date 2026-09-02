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

#ifndef LENSOR_OS_FILE_METADATA_H
#define LENSOR_OS_FILE_METADATA_H

#include <integers.h>
#include <storage/filesystem_driver.h>
#include <storage/storage_device_driver.h>

#include <memory>
#include <string>
#include <utility>

struct FileMetadata {
    enum class FileType : u32 {
        Regular,
        Directory,
        Pipe,
        Socket,
        // TODO: More file types (device, etc)
    };

    FileMetadata()
        : FileSize(-1ull), Name(""), FileDriver(nullptr), DriverData(nullptr), Invalid(true) {}

    FileMetadata(FileType type, std::string name, std::shared_ptr<FilesystemDriver> file_driver, u64 file_size, void* driver_data)
        : Type(type), FileSize(file_size), Name(std::move(name)), FileDriver(std::move(file_driver)), DriverData(driver_data), Invalid(false) {}

    ~FileMetadata() {
        // std::print("Closing FileMetadata \"{}\"\n", Name);
        if (FileDriver) FileDriver->close(this);
    }

    static std::shared_ptr<FileMetadata> Make(FileType type, std::string name, std::shared_ptr<FilesystemDriver> file_driver, u64 file_size, void* driver_data) {
        return std::make_shared<FileMetadata>(type, name, file_driver, file_size, driver_data);
    }

    usz offset{0};

    auto name() -> std::string_view { return Name; }
    auto invalid() -> bool { return Invalid; }
    auto filesystem_driver() -> std::shared_ptr<FilesystemDriver> { return FileDriver; }
    auto file_size() -> u64 { return FileSize; }
    auto driver_data() -> void* { return DriverData; }
    auto type() -> FileType { return Type; }

    bool is_regular() { return Type == FileType::Regular; }
    bool is_directory() { return Type == FileType::Directory; }

   private:
    FileType Type{FileType::Regular};
    usz FileSize{-1ull};
    std::string Name{""};

    // The driver is used for reading and writing from and to the file.
    std::shared_ptr<FilesystemDriver> FileDriver{nullptr};
    // Driver-specific data.
    void* DriverData{nullptr};

    bool Invalid = true;
};

struct DirectoryEntry {
    FileMetadata::FileType type;
    char name[252];
};

static_assert(sizeof(DirectoryEntry) == 256);

#endif /* LENSOR_OS_FILE_METADATA_H */
