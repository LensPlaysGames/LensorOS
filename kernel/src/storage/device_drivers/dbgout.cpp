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

#include <storage/device_drivers/dbgout.h>

#include <system.h>
#include <storage/file_metadata.h>
#include <virtual_filesystem.h>

auto DbgOutDriver::open(std::string_view) -> std::shared_ptr<FileMetadata> {
    return std::make_shared<FileMetadata>("stdout", sdd(SYSTEM->virtual_filesystem().StdoutDriver), 0ull, nullptr);
}
