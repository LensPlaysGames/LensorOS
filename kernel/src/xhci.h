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
 * along with LensorOS. If not, see <https://www.gnu.org/licenses/>
 */

#ifndef LENSOROS_XHCI_H
#define LENSOROS_XHCI_H

#include <bitops.h>

/// Capability Register Length
/// 8 bits
#define XHCI_CAPREGOFF_CAPLENGTH 0
/// Interface Version Number
/// 16 bits
#define XHCI_CAPREGOFF_HCIVERSION 0x02
/// Structural Parameters 1
/// 32 bits
#define XHCI_CAPREGOFF_HCSPARAMS1 0x04
/// Structural Parameters 2
/// 32 bits
#define XHCI_CAPREGOFF_HCSPARAMS2 0x08
/// Structural Parameters 3
/// 32 bits
#define XHCI_CAPREGOFF_HCSPARAMS3 0x0c
/// Capability Parameters 1
/// 32 bits
#define XHCI_CAPREGOFF_HCCPARAMS1 0x10
/// Doorbell Offset
/// 32 bits
#define XHCI_CAPREGOFF_DBOFF 0x14
/// Runtime Register Space Offset
/// 32 bits
#define XHCI_CAPREGOFF_RTSOFF 0x18
/// Capability Parameters 2
/// 32 bits
#define XHCI_CAPREGOFF_HCCPARAMS2 0x1c

#define XHCI_HCSPARAMS1_NUM_DEVICE_SLOTS_BITOFF 0
#define XHCI_HCSPARAMS1_NUM_DEVICE_SLOTS_MASK GENMASK(7, XHCI_HCSPARAMS1_NUM_DEVICE_SLOTS_BITOFF)
#define XHCI_HCSPARAMS1_NUM_INTERRUPTERS_BITOFF 8
#define XHCI_HCSPARAMS1_NUM_INTERRUPTERS_MASK GENMASK(18, XHCI_HCSPARAMS1_NUM_INTERRUPTERS_BITOFF)
#define XHCI_HCSPARAMS1_NUM_PORTS_BITOFF 24
#define XHCI_HCSPARAMS1_NUM_PORTS_MASK GENMASK(31, XHCI_HCSPARAMS1_NUM_DEVICE_SLOTS_BITOFF)

/// 1 << this value == maximum value supported the Event Ring Segment Table
/// Base Size registers
#define XHCI_HCSPARAMS2_EVENT_RING_SEGMENT_TABLE_MAX_BITOFF 4
#define XHCI_HCSPARAMS2_EVENT_RING_SEGMENT_TABLE_MAX_MASK GENMASK(7, XHCI_HCSPARAMS2_EVENT_RING_SEGMENT_TABLE_MAX_BITOFF)

#define XHCI_HCSPARAMS2_SCRATCHPAD_BUFFERS_HI_BITOFF 21
#define XHCI_HCSPARAMS2_SCRATCHPAD_BUFFERS_HI_MASK GENMASK(25, XHCI_HCSPARAMS2_SCRATCHPAD_BUFFERS_HI_BITOFF)
#define XHCI_HCSPARAMS2_SCRATCHPAD_BUFFERS_LO_BITOFF 27
#define XHCI_HCSPARAMS2_SCRATCHPAD_BUFFERS_LO_MASK GENMASK(31, XHCI_HCSPARAMS2_SCRATCHPAD_BUFFERS_LO_BITOFF)

#define XHCI_HCCPARAMS1_EXTENDED_CAPS_BITOFF 16
#define XHCI_HCCPARAMS1_EXTENDED_CAPS_MASK GENMASK(31, XHCI_HCCPARAMS1_EXTENDED_CAPS_BITOFF)
#define XHCI_HCCPARAMS1_MAX_STREAMS_BITOFF 12
#define XHCI_HCCPARAMS1_MAX_STREAMS_MASK GENMASK(15, XHCI_HCCPARAMS1_MAX_STREAMS_BITOFF)
#define XHCI_HCCPARAMS1_CONTIGUOUS_FRAME_ID BIT(11)
#define XHCI_HCCPARAMS1_STOPPED_EDTLA BIT(10)
#define XHCI_HCCPARAMS1_STOPPED_SHORT_PACKET BIT(9)
#define XHCI_HCCPARAMS1_PARSE_ALL_EVENT_TRB BIT(8)
#define XHCI_HCCPARAMS1_NO_SECONDARY_STREAM_ID BIT(7)
#define XHCI_HCCPARAMS1_LATENCY_TOLERANT_MSG BIT(6)
#define XHCI_HCCPARAMS1_LIGHT_HC_RESET BIT(5)
#define XHCI_HCCPARAMS1_PORT_INDICATORS BIT(4)
#define XHCI_HCCPARAMS1_PORT_POWER_CONTROL BIT(3)
#define XHCI_HCCPARAMS1_CONTEXT_IS64 BIT(2)
#define XHCI_HCCPARAMS1_BANDWIDTH_NEGOTIATION BIT(1)
#define XHCI_HCCPARAMS1_IS64 BIT(0)

/// USB Command
#define XHCI_OPREGOFF_USBCMD 0
/// USB Status
#define XHCI_OPREGOFF_USBSTS 0x04
/// Page Size
#define XHCI_OPREGOFF_PAGESIZE 0x08
/// Device Notification Control
#define XHCI_OPREGOFF_DNCTRL 0x14
/// Command Ring Control
#define XHCI_OPREGOFF_CRCR 0x18
/// Device Context Base Address Array Pointer
#define XHCI_OPREGOFF_DCBAAP 0x30
/// Configure
#define XHCI_OPREGOFF_CONFIG 0x38

/// 0 -> stopped
/// 1 -> running
#define XHCI_USBCMD_RUN BIT(0)
#define XHCI_USBCMD_RESET BIT(1)
#define XHCI_USBCMD_INTERRUPTER_ENABLE BIT(2)
#define XHCI_USBCMD_HOST_SYSTEM_ERROR_ENABLE BIT(3)
#define XHCI_USBCMD_LIGHT_RESET BIT(7)
#define XHCI_USBCMD_SAVE_STATE BIT(8)
#define XHCI_USBCMD_RESTORE_STATE BIT(9)
#define XHCI_USBCMD_WRAP_EVENT_ENABLE BIT(10)
#define XHCI_USBCMD_U3_MFINDEX_ENABLE BIT(11)
#define XHCI_USBCMD_CONFIG_ENDPOINT_MAX_LATENCY_TOO_LARGE_ENABLE BIT(13)
#define XHCI_USBCMD_EXT_TBC_ENABLE BIT(14)
#define XHCI_USBCMD_EXT_TBC_TRB_STATUS_ENABLE BIT(15)
#define XHCI_USBCMD_VTIO_ENABLE BIT(16)

#define XHCI_PAGESIZE_BITOFF 0
/// Supports a page size of 2^(N+12) if bit N is set (1).
#define XHCI_PAGESIZE_MASK GENMASK(15, XHCI_PAGESIZE_BITOFF)

#define XHCI_CRCR_STOP BIT(1)
#define XHCI_CRCR_ABORT BIT(2)
#define XHCI_CRCR_ISRUNNING BIT(3)
/// Command Ring Pointer (RW) -- defaults to 0. This field defines high order
/// bits of the initial value of the 64-bit Command Ring Dequeue Pointer.
#define XHCI_CRCR_BASE GENMASK(63, 6)

/// PORTSC
#define XHCI_PORTREGOFF_STATUS_CONTROL 0
/// PORTPMSC Port Power Management Status and Control
#define XHCI_PORTREGOFF_POWER_MGMT 0x4
/// PORTLI
#define XHCI_PORTREGOFF_LINK_INFO 0x8
/// PORTHLPMC
#define XHCI_PORTREGOFF_HW_LPM_CONTROL 0xc

#endif  // LENSOROS_XHCI_H
