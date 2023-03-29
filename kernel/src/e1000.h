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
    PCI::PCIHeader0* PCIHeader {nullptr};
    PCI::BarType BARType;

    bool EEPROMExists{false};
    bool detect_eeprom();

    union {
        usz BARMemoryAddress;
        usz BARIOAddress;
    };

    u8 MACAddress[6] {0};

    void write_command(u16 address, u32 value);
    u32 read_command(u16 address);

    u16 read_eeprom(u8 address);

    // Requires BARType and EEPROM detection to be done already!
    // FIXME: Store "state" that e1000 is currently in, and assert that
    // state is >= expected state to make sure that these things *are*
    // initialised.
    void get_mac_address();

public:
    E1000() {}
    E1000(PCI::PCIHeader0* header);
};

extern E1000 gE1000;

#endif /* LENSOR_OS_E1000_H */
