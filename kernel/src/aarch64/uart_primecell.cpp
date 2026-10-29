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

#include <stdint.h>
#include <uart.h>

namespace UART {
uintptr_t Base{0};
bool Initialized{false};
bool initialized() {
    return Initialized and Base != 0;
};

void initialize() {
    Initialized = true;
}

// UART data register
volatile uint8_t* uartdr() { return (uint8_t*)(Base + 0); }
// UART flags register
volatile uint32_t* uartfr() { return (uint32_t*)(Base + 12); }

// Called from the UART interrupt handler when data is received.
uint8_t read() {
    if (not initialized()) return 0;

    uint maxSpins = 1000000u;
    // bit 6 -- receive FIFO empty
    while ((*uartfr() & (1u << 6)) and maxSpins > 0)
        maxSpins--;

    if (maxSpins == 0)
        return 0;

    return *uartdr();
}

void out_raw(uint8_t byte) {
    if (initialized() == false)
        return;

    // Wait for UART chip to empty it's transmit buffer before potentially
    // over-writing data that was not yet sent, or causing a "FIFO written
    // while full" error.
    uint maxSpins = 1000000u;
    // bit 5 -- transmit FIFO full
    while ((*uartfr() & (1u << 5)) and maxSpins > 0)
        maxSpins--;

    if (maxSpins == 0)
        return;

    *uartdr() = byte;
}

}  // namespace UART
