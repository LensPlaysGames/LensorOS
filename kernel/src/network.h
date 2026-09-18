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

#ifndef LENSOROS_NETWORK_H
#define LENSOROS_NETWORK_H

#include <integers.h>

#include <bit>

namespace Network {

// network byte order is *always* big endian.
// host byte order may either be little or big endian.
template <typename T>
constexpr inline T host_to_network(T host_value) {
    if (std::endian::native == std::endian::little)
        return std::byteswap(host_value);
    return host_value;
}
// network byte order is *always* big endian.
// host byte order may either be little or big endian.
template <typename T>
constexpr inline T network_to_host(T network_value) {
    if (std::endian::native == std::endian::little)
        return std::byteswap(network_value);
    return network_value;
}

constexpr u16 ETHERTYPE_ARP = host_to_network(u16(0x0806));

struct EthernetFrameHeader {
    u8 MACDestination[6];
    u8 MACSource[6];
    u16 Ethertype;
} __attribute__((packed));
static_assert(sizeof(EthernetFrameHeader) == 14, "");

constexpr u16 ARP_HARDWARETYPE_ETHERNET = Network::host_to_network(u16(1));
constexpr u16 ARP_PROTOCOLTYPE_IPV4 = Network::host_to_network(u16(0x0800));

struct ARPData {
    /// HTYPE
    u16 HardwareType;
    /// PTYPE
    u16 ProtocolType;
    /// HLEN
    u8 HardwareLength;
    /// PLEN
    u8 ProtocolLength;
    /// OPER
    /// 1 == request
    /// 2 == reply
    u16 Operation;
    /// SHA
    u8 SenderHardwareAddress[6];
    /// SPA
    /// "Internetwork address"
    u8 SenderProtocolAddress[4];
    u8 TargetHardwareAddress[6];
    u8 TargetProtocolAddress[4];
} __attribute__((packed));
static_assert(sizeof(ARPData) == 28, "");

}  // namespace Network

#endif /* LENSOROS_NETWORK_H */
