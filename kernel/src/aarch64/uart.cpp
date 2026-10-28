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

#include <uart.h>

namespace UART {
bool Initialized{false};
bool initialized() {
    return Initialized;
};

void initialize();

enum class Chip;
const char* get_uart_chip_name(Chip);

// Called from the UART interrupt handler when data is received.
uint8_t read() {
    if (not initialized()) return 0;

    uint8_t r{};
    return r;
}

void out_raw(uint8_t byte) {
    if (initialized() == false)
        return;

    // TODO
}

}  // namespace UART
