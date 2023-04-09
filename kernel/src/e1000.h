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

#ifndef LENSOR_OS_E1000_H
#define LENSOR_OS_E1000_H

#include <integers.h>
#include <pci.h>

class E1000 {
    struct RXDesc {
        volatile u64 Address;
        volatile u16 Length;
        volatile u16 Checksum;
        enum {
            /// Indicates whether hardware is done with the descriptor.
            /// When set along with EOP (bit 1), the received packet is
            /// complete in main memory
            DONE = (1 << 0),
            END_OF_PACKET = (1 << 1),
            /// Reads as 1.
            IGNORE_CHECKSUM_INDICATION = (1 << 2),
            /// Reads as 0.
            IS_8021Q = (1 << 3),
            TCP_CHECKSUM_CALCULATED = (1 << 5),
            IP_CHECKSUM_CALCULATED = (1 << 6),
            PASSED_IN_EXACT_FILTER = (1 << 7),
        };
        volatile u8 Status;
        enum {
            /// CRC errors and alignment errors are both indicated via the CE bit.
            /// Software may distinguish between these errors by monitoring the
            /// respective statistics registers.
            CRC_OR_ALIGNMENT = (1 << 0),
            /// When set, indicates a packet received with bad symbol. Applicable
            /// only in TBI mode/internal SerDes.
            SYMBOL = (1 << 1),
            /// When set, indicates a received packet with a bad delimiter sequence
            /// (in TBI mode/ internal SerDes). In other 802.3 implementations,
            /// this would be classified as a framing error.
            /// A valid delimiter sequence consists of:
            ///     idle -> start-of-frame (SOF) -> data, -> pad (optional)
            ///          -> end-of-frame (EOF) -> fill (optional) -> idle
            SEQUENCE = (1 << 2),
            /// When set, indicates a packet was received in which the carrier
            /// extension error was signaled across the GMII interface. A carrier
            /// extension error is signaled by the PHY by the encoding of 1Fh on
            /// the receive data inputs while I_RX_ER is asserted.
            /// Valid only while working in 1000 Mb/s half-duplex mode of operation.
            /// This bit is reserved for all Ethernet controllers except the 82544GC/EI.
            CARRIER_EXTENSION = (1 << 4),
            TCP_UDP_CHECKSUM = (1 << 5),
            IP_CHECKSUM = (1 << 6),
            /// Indicates that a data error occurred during the packet reception. A
            /// data error in TBIa mode (82544GC/EI)/internal SerDes (82546GB/EB
            /// and 82545GM/EM) refers to the reception of a /V/ code (see Section
            /// 8.2.1.3). In GMII or MII mode, the assertion of I_RX_ER during data
            /// reception indicates a data error. This bit is valid only when the
            /// EOP and DD bits are set; it is not set in descriptors unless RCTL.
            /// SBP (Store Bad Packets) control bit is set.
            DATA = (1 << 7),
        };
        volatile u8 Errors;
        volatile u16 Special;
    } __attribute__((packed));

    struct TXDesc {
        volatile u64 Address;
        volatile u16 Length;
        /// The Checksum offset field indicates where, relative to the start of the packet,
        /// to insert a TCP checksum if this mode is enabled (Insert Checksum bit (IC) is set in TDESC.CMD).
        /// Hardware ignores CSO unless EOP is set in TDESC.CMD. CSO is provided in unit of bytes and must be
        /// in the range of the data provided to the Ethernet controller in the descriptor. (CSO < length - 1).
        /// Should be written with 0b for future compatibility.
        volatile u8 ChecksumOffset;
        volatile u8 Command;
        enum Status {
            DONE = (1 << 0),
            EXCESS_COLLISIONS = (1 << 1),
            LATE_COLLISION = (1 << 2),
            UNDERRUN = (1 << 3),
        };
        volatile u8 Status;
        /// The Checksum start field (TDESC.CSS) indicates where to begin computing
        /// the checksum. The software must compute this offset to back out the bytes
        /// that should not be included in the TCP checksum. CSS is provided in units
        /// of bytes and must be in the range of data provided to the Ethernet controller
        /// in the descriptor (CSS < length). For short packets that ar padded by the
        /// software, CSS must be in the range of the unpadded data length. A value of
        /// 0b corresponds to the first byte in the packet.
        /// CSS must be set in the first descriptor of the packet.
        volatile u8 ChecksumStartField;
        volatile u16 Special;
    } __attribute__((packed));

    PCI::PCIHeader0* PCIHeader { nullptr };

    uint RXDescCount { 0 };
    volatile RXDesc* RXDescPhysical { nullptr };
    uint TXDescCount { 0 };
    volatile TXDesc* TXDescPhysical { nullptr };

    /// Tagged union
    PCI::BarType BARType;
    union {
        usz BARMemoryAddress;
        usz BARIOAddress;
    };

    bool EEPROMExists{false};

    /// Attempt to decode BAR address from PCIHeader.
    /// Sets BARType and a corresponding value in the union.
    void decode_base_address();

    /// Set EEPROMExists member to true iff an EEPROM is present.
    /// First, if possible, determine presence by checking if
    /// EECD.EE_PRES bit is set. On some cards, this isn't possible, so
    /// we just attempt a read and see if it succeeds.
    /// EECD == EEPROM Control and Data register (0x10)
    /// EE_PRES == EEPROM Present (bit 8)
    void detect_eeprom();

    void write_command(u16 address, u32 value);
    u32 read_command(u16 address);

    u16 read_eeprom(u8 address);

    // Requires BARType and EEPROM detection to be done already!
    // FIXME: Store "state" that e1000 is currently in, and assert that
    // state is >= expected state to make sure that these things *are*
    // initialised.
    void get_mac_address();

    /// Configure the device through the device control register.
    void configure_device();

    void initialise_rx();
    void initialise_tx();

    /// The known "head" index of the transmit descriptor ring buffer.
    /// This number will trail behind the hardware head index; for
    /// every index that is incremented past until the hardware head
    /// index is reached, that transmit descriptor will be deleted.
    /// This will occur every time we get a TX queue empty interrupt.
    uint RXHead {0};

    /// The known "head" index of the transmit descriptor ring buffer.
    /// This number will trail behind the hardware head index; for
    /// every index that is incremented past until the hardware head
    /// index is reached, that transmit descriptor will be deleted.
    /// This will occur every time we get a TX queue empty interrupt.
    uint TXHead {0};

public:
    u8 MACAddress[6] {0};

    E1000() {}
    E1000(PCI::PCIHeader0* header);
    void handle_interrupt();
    uint irq_number();
    uint interrupt_line();
    void write_raw(void* data, usz length);
};

extern E1000 gE1000;

#endif /* LENSOR_OS_E1000_H */
