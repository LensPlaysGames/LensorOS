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
        volatile u8 CSO;
        volatile u8 Command;
        volatile u8 Status;
        volatile u8 CSS;
        volatile u16 Special;
    } __attribute__((packed));

    enum E1000State {
        UNRECOVERABLE,
        UNINITIALISED,
        BASE_ADDRESS_DECODED,
        RESET,
        EEPROM_PROBED,
        MAC_ADDRESS_DECODED,
        RX_DESCS_INITIALISED,
        TX_DESCS_INITIALISED,
        INTERRUPTS_ENABLED,
        INITIALISED,
    } State {E1000::UNINITIALISED};

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

    u8 MACAddress[6] {0};

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

public:
    E1000() {}
    E1000(PCI::PCIHeader0* header);
    E1000State state() { return State; }
};

extern E1000 gE1000;

#endif /* LENSOR_OS_E1000_H */
