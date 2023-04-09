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
        volatile u8 Status;
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
