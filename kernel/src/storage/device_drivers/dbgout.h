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

#ifndef LENSOR_OS_DBGOUT_DRIVER_H
#define LENSOR_OS_DBGOUT_DRIVER_H

#include <debug.h>
#include <integers.h>
#include <storage/storage_device_driver.h>

class DbgOutDriver final : public StorageDeviceDriver {
public:
    ssz read(usz, usz, void*) { return -1; };
    ssz write(usz byteOffset, usz byteCount, void* buffer) {
        dbgmsg_buf(reinterpret_cast<u8*>(buffer) + byteOffset, byteCount);
        return byteCount;
    };
};

#endif /* LENSOR_OS_DBGOUT_DRIVER_H */
