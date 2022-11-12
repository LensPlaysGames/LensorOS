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

#ifndef LENSOR_OS_AHCI_H
#define LENSOR_OS_AHCI_H

#include <integers.h>
#include <system.h>

/// Size of AHCI Port buffer (how much the hardware reads/writes at a time)
/// 128mib = 134217700 bytes = 32768 pages = 0x8000
/// 1mib = 1048576 bytes = 256 pages = 0x100
#define MAX_READ_PAGES 0x100
#define MAX_READ_BYTES (MAX_READ_PAGES * 0x1000)

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ  0x08

#define ATA_CMD_WRITE_DMA        0xca
#define ATA_CMD_WRITE_DMA_QUEUED 0xcc
#define ATA_CMD_WRITE_MULTIPLE   0xc5
#define ATA_CMD_WRITE_SECTORS    0x30

#define ATA_CMD_READ_DMA        0xc8
#define ATA_CMD_READ_DMA_QUEUED 0xc7
#define ATA_CMD_READ_MULTIPLE   0xc4
#define ATA_CMD_READ_SECTORS    0x20

#define ATA_CMD_WRITE_DMA_EXT        0x35
#define ATA_CMD_WRITE_DMA_QUEUED_EXT 0x36
#define ATA_CMD_WRITE_MULTIPLE_EXT   0x39
#define ATA_CMD_WRITE_SECTORS_EXT    0x34

#define ATA_CMD_READ_DMA_EXT        0x25
#define ATA_CMD_READ_DMA_QUEUED_EXT 0x26
#define ATA_CMD_READ_MULTIPLE_EXT   0x29
#define ATA_CMD_READ_SECTORS_EXT    0x24

#define ATA_CMD_PACKET       0xa0
#define ATA_CMD_DEVICE_RESET 0x08

#define ATA_CMD_SERVICE 0xa2
#define ATA_CMD_NOP     0

#define SCSI_CMD_TEST_READY                     0x00
#define SCSI_CMD_REQUEST_SENSE                  0x03
#define SCSI_CMD_FORMAT                         0x04
#define SCSI_CMD_INQUIRY                        0x12
#define SCSI_CMD_START_STOP                     0x1b
#define SCSI_CMD_PREVENT_REMOVAL                0x1e
#define SCSI_CMD_READ_FORMAT_CAPACITIES         0x23
#define SCSI_CMD_READ_CAPACITY                  0x25
#define SCSI_CMD_READ_10                        0x28
#define SCSI_CMD_WRITE_10                       0x2a
#define SCSI_CMD_SEEK_10                        0x2b
#define SCSI_CMD_WRITE_AND_VERIFY_10            0x2e
#define SCSI_CMD_VERIFY_10                      0x2f
#define SCSI_CMD_SYNC_CACHE                     0x35
#define SCSI_CMD_WRITE_BUFFER                   0x3b
#define SCSI_CMD_READ_BUFFER                    0x3c
#define SCSI_CMD_READ_TOC_PMA_ATIP              0x43
#define SCSI_CMD_GET_CONFIG                     0x46
#define SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION  0x4a
#define SCSI_CMD_READ_DISC_INFORMATION          0x51
#define SCSI_CMD_READ_TRACK_INFORMATION         0x52
#define SCSI_CMD_RESERVE_TRACK                  0x53
#define SCSI_CMD_SEND_OPC_INFORMATION           0x54
#define SCSI_CMD_MODE_SELECT_10                 0x55
#define SCSI_CMD_REPAIR_TRACK                   0x58
#define SCSI_CMD_MODE_SENSE_10                  0x5a
#define SCSI_CMD_CLOSE_TRACK_SESSION            0x5b
#define SCSI_CMD_READ_BUFFER_CAPACITY           0x5c
#define SCSI_CMD_SEND_CUE_SHEET                 0x5d
#define SCSI_CMD_READ_16                        0x88
#define SCSI_CMD_WRITE_16                       0x8a
#define SCSI_CMD_VERIFY_16                      0x8f
#define SCSI_CMD_WRITE_SAME_16                  0x93
#define SCSI_CMD_REPORT_LUNS                    0xa0
#define SCSI_CMD_BLANK                          0xa1
#define SCSI_CMD_SECURITY_PROTOCOL_IN           0xa2
#define SCSI_CMD_SEND_KEY                       0xa3
#define SCSI_CMD_REPORT_KEY                     0xa4
#define SCSI_CMD_LOAD_UNLOAD_MEDIUM             0xa6
#define SCSI_CMD_SET_READ_AHEAD                 0xa7
#define SCSI_CMD_READ_12                        0xa8
#define SCSI_CMD_WRITE_12                       0xaa
#define SCSI_CMD_GET_PERFORMANCE                0xac
#define SCSI_CMD_READ_DISC_STRUCTURE            0xad
#define SCSI_CMD_SECURITY_PROTOCOL_OUT          0xb5
#define SCSI_CMD_SET_STREAMING                  0xb6
#define SCSI_CMD_READ_CD_MSF                    0xb9
#define SCSI_CMD_SET_CD_SPEED                   0xbb
#define SCSI_CMD_MECHANISM_STATUS               0xbd
#define SCSI_CMD_READ_CD                        0xbe
#define SCSI_CMD_SEND_DISC_STRUCTURE            0xbf

#define HBA_PORT_DEVICE_PRESENT 0x3
#define HBA_PORT_IPM_ACTIVE     0x1

// Px == Port x where x is 0-31 inclusive.

// Command list Running (DMA active)
#define HBA_PxCMD_CR   0x8000
// FIS Recieve Running
#define HBA_PxCMD_FR   0x4000
// FIS Recieve Enable
#define HBA_PxCMD_FRE  0x10
// Start DMA
#define HBA_PxCMD_ST   1

#define HBA_PxIS_TFES (1 << 30)

constexpr u32 HBA_PRDT_INTERRUPT_ON_COMPLETION = (1ul << 31);

#define SATA_SIG_ATAPI 0xeb140101
#define SATA_SIG_ATA   0x00000101
#define SATA_SIG_SEMB  0xc33c0101
#define SATA_SIG_PM    0x96690101

/// AHCI (Advance Host Controller Interface) developed by Intel
///   Facilitates handling of Serial ATA devices.
///   An AHCI Controller is referred to as a Host Bus Adapter, or HBA.
///   The HBA's job (nutshell) is to allow access to SATA drives using
///     system memory and memory mapped registers, without using what
///     are called task files (a requirement for IDE, the alternative).
///   The HBA does this through the use of ports. A port is a pathway
///     for communication between a host (the computer) and a SATA device.
///   An HBA may support up to 32 ports which can attach different
///     SATA devices (disk drives, port multipliers, etc).
///   By sending commands through these ports, the devices can be manipulated
///     to do anything they are capable of (read data, write data, etc).
///
///   NOTE: All memory addresses used by the HBA must be physical.
///         This is done by mapping them 1:1 using the PTM.

namespace AHCI {
    /// Host Bus Adapter Port
    struct HBAPort {
        u32 CommandListBaseAddress;
        u32 CommandListBaseAddressUpper;
        u32 FISBaseAddress;
        u32 FISBaseAddressUpper;
        u32 InterruptStatus;
        u32 InterruptEnable;
        u32 CommandAndStatus;
        u32 Reserved0;
        u32 TaskFileData;
        u32 signature;
        u32 SataStatus;
        u32 SataControl;
        u32 SataError;
        u32 SataActive;
        u32 CommandIssue;
        u32 SataNotification;
        u32 FISSwitchControl;
        u32 Reserved1[11];
        u32 Vendor[4];

        void set_command_list_base(void* commandListBaseAddress) volatile {
            u64 commandListBase = reinterpret_cast<u64>(commandListBaseAddress);
            CommandListBaseAddress = static_cast<u32>(commandListBase);
            CommandListBaseAddressUpper = static_cast<u32>(commandListBase >> 32);
        }

        void set_frame_information_structure_base(void* fisBaseAddress) volatile {
            u64 fisBase = reinterpret_cast<u64>(fisBaseAddress);
            FISBaseAddress = static_cast<u32>(fisBase);
            FISBaseAddressUpper = static_cast<u32>(fisBase >> 32);
        }

        u64 command_list_base() volatile {
            return static_cast<u64>(CommandListBaseAddress)
                + (static_cast<u64>(CommandListBaseAddressUpper) << 32);
        }

        u64 frame_information_structure_base() volatile {
            return static_cast<u64>(FISBaseAddress)
                + (static_cast<u64>(FISBaseAddressUpper) << 32);
        }
    };

    /// Host Bus Adapter Memory Registers
    ///   The layout of the memory registers
    ///     accessable through the Host Bus Adapter.
    struct HBAMemory {
        u32 HostCapability;
        u32 GlobalHostControl;
        u32 InterruptStatus;
        u32 PortsImplemented;
        u32 Version;
        u32 cccControl;
        u32 cccPorts;
        u32 EnclosureManagementLocation;
        u32 EnclosureManagementControl;
        u32 HostCapabilitiesExtended;
        u32 BIOSHandoffControlStatus;
        u8 Reserved0[0x74];
        u8 Vendor[0x60];
        HBAPort Ports[1];
    };

    /// Host Bus Adapter Command Header
    /// The beginning of a Host Bus Adapter command is structured as shown.
    // FIXME: Get rid of bitfields! We would just have enough space for
    // everything, then use functions that & and >> properly to make
    // everything work. Not too difficult.
    struct HBACommandHeader {
        u8 CommandFISLength :5;
        u8 ATAPI            :1;
        u8 Write            :1;
        u8 Prefetchable     :1;
        u8 Reset            :1;
        u8 BIST             :1;
        u8 ClearBusy        :1;
        u8 Reserved0        :1;
        u8 PortMultiplier   :4;
        u16 PRDTLength;
        u32 PRDBCount;
        u32 CommandTableBaseAddress;
        u32 CommandTableBaseAddressUpper;
        u32 Reserved1[4];

        void set_command_table_base(u64 commandTableBase) {
            CommandTableBaseAddress = static_cast<u32>(commandTableBase);
            CommandTableBaseAddressUpper = static_cast<u32>(commandTableBase >> 32);
        }

        void set_command_table_base(void* commandTableBaseAddress) {
            set_command_table_base(reinterpret_cast<u64>(commandTableBaseAddress));
        }

        u64 command_table_base() {
            return static_cast<u64>(CommandTableBaseAddress)
                + (static_cast<u64>(CommandTableBaseAddressUpper) << 32);
        }
    };

    /// Host Bus Adapter Physical Region Descriptor Table Entry
    ///   Specifies data payload address in memory as well as size.
    struct HBA_PRDTEntry {
        u32 DataBaseAddress;
        u32 DataBaseAddressUpper;
        u32 Reserved0;
        /* 0b00000000000000000000000000000000
         *             ======================  Byte Count
         *    =========                        Reserved1
         *   =                                 Int. on Completion
         */
        u32 Information;


        void set_data_base(u64 newDataBaseAddress) {
            DataBaseAddress = static_cast<u32>(newDataBaseAddress);
            DataBaseAddressUpper = static_cast<u32>(newDataBaseAddress >> 32);
        }

        void set_byte_count(u32 newByteCount) {
            // Clear byte count bits to zero.
            Information &= ~0b1111111111111111111111;
            // Set byte count bits from new byte count.
            Information |= newByteCount & 0b1111111111111111111111;
        }

        void set_interrupt_on_completion(bool value) {
            // Clear the int. on completion bit.
            Information &= ~(HBA_PRDT_INTERRUPT_ON_COMPLETION);
            // Set the int. on completion bit if passed value is true.
            if (value)
                Information |= HBA_PRDT_INTERRUPT_ON_COMPLETION;
        }

        u64 data_base() {
            return static_cast<u64>(DataBaseAddress)
                + (static_cast<u64>(DataBaseAddressUpper) << 32);
        }

        u32 byte_count() {
            return Information & (0b1111111111111111111111);
        }

        bool interrupt_on_completion() {
            return Information & HBA_PRDT_INTERRUPT_ON_COMPLETION;
        }
    };

    struct HBACommandTable {
        u8 CommandFIS[64];
        u8 ATAPICommand[16];
        u8 Reserved[48];
        // FIXME: Is this supposed to be a variable length array?
        HBA_PRDTEntry PRDTEntry[];
    };

    /// Frame Information Structure Type
    enum FIS_TYPE {
        /// Used by the host to send command or control to a device.
        REG_H2D   = 0x27,
        /// Used by the device to notify the host that some ATA
        /// register has changed.
        REG_D2H   = 0x34,
        DMA_ACT   = 0x39,
        DMA_SETUP = 0x41,
        /// Used by both the host and device to send data payload.
        DATA      = 0x46,
        BIST      = 0x58,
        /// Used by the device to notify the host that it's about to
        /// send (or ready to recieve) a PIO data payload.
        PIO_SETUP = 0x5f,
        DEV_BITS  = 0xa1,
    };

    /// Frame Information Structure Reegister Host to Device
    struct FIS_REG_H2D {
        // FIXME: Get rid of bitfields! Same situation here as above;
        // replace bitfields with fields that are large enough to store
        // them, then just write functions to access them.
        u8 Type;
        u8 PortMultiplier:4;
        u8 Reserved0:3;
        u8 CommandControl:1;
        u8 Command;
        u8 FeatureLow;
        u8 LBA0;
        u8 LBA1;
        u8 LBA2;
        u8 DeviceRegister;
        u8 LBA3;
        u8 LBA4;
        u8 LBA5;
        u8 FeatureHigh;
        u8 CountLow;
        u8 CountHigh;
        u8 ISOCommandCompletion;
        u8 Control;
        u8 Reserved1[4];

        void set_logical_block_addresses(u64 sector) {
            u32 sectorLow = static_cast<u32>(sector);
            u32 sectorHigh = static_cast<u32>(sector >> 32);
            LBA0 = static_cast<u8>(sectorLow);
            LBA1 = static_cast<u8>(sectorLow >> 8);
            LBA2 = static_cast<u8>(sectorLow >> 16);
            LBA3 = static_cast<u8>(sectorHigh);
            LBA4 = static_cast<u8>(sectorHigh >> 8);
            LBA5 = static_cast<u8>(sectorHigh >> 16);
        }

        void set_feature(u16 newFeature) {
            FeatureLow = static_cast<u8>(newFeature);
            FeatureHigh = static_cast<u8>(newFeature >> 8);
        }

        void set_count(u16 newCount) {
            CountLow = static_cast<u8>(newCount);
            CountHigh = static_cast<u8>(newCount >> 8);
        }

        u16 feature() {
            return static_cast<u16>(FeatureLow)
                + (static_cast<u16>(FeatureHigh) << 8);
        }

        u16 count() {
            return static_cast<u16>(CountLow)
                + (static_cast<u16>(CountHigh) << 8);
        }
    };

    /// Port Type
    /// DO NOT REORDER THESE UNLESS YOU ALSO REORDER PORT TYPE STRINGS ARRAY.
    enum PortType {
        None = 0,
        SATA = 1,
        SEMB = 2,
        PM = 3,
        SATAPI = 4
    };
    // This would need reordered if PortType is reordered.
    extern const char* port_type_strings[5];
    const char* port_type_string(PortType);

    PortType get_port_type(HBAPort* port);
}

#endif /* LENSOR_OS_AHCI_H */
