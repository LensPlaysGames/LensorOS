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

#ifndef LENSOR_OS_HIGH_LEVEL_MEMORY_TYPES_H
#define LENSOR_OS_HIGH_LEVEL_MEMORY_TYPES_H

#include <memory/common.h>
#include <stdint.h>

struct VirtualAddress;
struct PhysicalAddress;
struct PhysicalKernelAddress;

struct VirtualAddress {
    uintptr_t address;
    explicit constexpr VirtualAddress(uintptr_t address) noexcept
        : address(address) {}
    explicit constexpr VirtualAddress(const volatile void* address) noexcept
        : address(uintptr_t(address)) {}
    explicit constexpr operator uintptr_t() const noexcept { return address; }
    VirtualAddress() = delete;
};
struct PhysicalAddress {
    uintptr_t address;
    explicit constexpr PhysicalAddress(uintptr_t address) noexcept
        : address(address) {}
    explicit constexpr PhysicalAddress(void* address) noexcept
        : address(uintptr_t(address)) {}
    explicit constexpr PhysicalAddress(const PhysicalKernelAddress address) noexcept;
    explicit constexpr operator uintptr_t() const noexcept { return address; }
    PhysicalAddress() = delete;
};
struct PhysicalKernelAddress {
    uintptr_t address;
    explicit constexpr PhysicalKernelAddress(uintptr_t address) noexcept
        : address(address) {}
    explicit constexpr PhysicalKernelAddress(const volatile void* address) noexcept
        : address(uintptr_t(address)) {}
    explicit constexpr PhysicalKernelAddress(const PhysicalAddress address) noexcept;
    explicit constexpr operator uintptr_t() const noexcept { return address; }
    constexpr operator VirtualAddress() const noexcept { return VirtualAddress(address); }
    PhysicalKernelAddress() = delete;
};

namespace Memory {

using memory_space_handle = uintptr_t;

extern memory_space_handle startup_handle;

constexpr inline PhysicalAddress TO_FRAME_POINTER(PhysicalKernelAddress p) {
    return PhysicalAddress(p.address - PHYSICAL_BASE);
}
constexpr inline PhysicalKernelAddress FROM_FRAME_POINTER(PhysicalAddress p) {
    return PhysicalKernelAddress(p.address + PHYSICAL_BASE);
}

enum class Flags : uint32_t {
    Readable = uint32_t(1u) << 0,
    Writable = uint32_t(1u) << 1,
    User = uint32_t(1u) << 2,
    // The CPU will not fetch from it's cache when reading from memory; it
    // will make sure to fetch directly from physical memory.
    DisableReadCache = uint32_t(1u) << 3,
    // The CPU will not *just* write to it's cache when writing to memory; it
    // will make sure to propogate changes directly to physical memory.
    DisableWriteCache = uint32_t(1u) << 4,
    CopyOnWrite = uint32_t(1u) << 31,
};
using Flag = Flags;
constexpr inline Flags operator|(const Flag a, const Flag b) noexcept {
    return Flags((uint32_t(a)) | ((uint32_t)b));
}
constexpr inline bool operator&(const Flag a, const Flag b) noexcept {
    return (uint32_t(a)) & ((uint32_t)b);
}
constexpr inline Flags operator|=(Flag& a, const Flag b) noexcept {
    a = a | b;
    return a;
}
constexpr inline Flags operator&=(Flag& a, const Flag b) noexcept {
    a = Flags(uint32_t(a) & uint32_t(b));
    return a;
}
constexpr inline Flags operator~(const Flag a) noexcept {
    return Flags(~(uint32_t(a)));
}

}  // namespace Memory

constexpr inline PhysicalAddress::PhysicalAddress(const PhysicalKernelAddress address) noexcept
    : address(Memory::TO_FRAME_POINTER(address).address) {}
constexpr inline PhysicalKernelAddress::PhysicalKernelAddress(const PhysicalAddress address) noexcept
    : address(Memory::FROM_FRAME_POINTER(address).address) {}

#endif
