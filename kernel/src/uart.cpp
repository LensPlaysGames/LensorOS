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

#include <cstr.h>
#include <io.h>

namespace UART {
    bool Initialized { false };
    bool initialized() {
        return Initialized;
    };

    enum class Chip {
        NONE = 0,
        _8250,
        _16450,
        _16550,
        _16550A,
        _16750,
    } chip { Chip::NONE };

    const char* get_uart_chip_name(Chip chip) {
        switch (chip) {
        case Chip::NONE:
            return "Invalid UART chip";
        case Chip::_8250:
            return "8250";
        case Chip::_16450:
            return "16450";
        case Chip::_16550:
            return "16550";
        case Chip::_16550A:
            return "16550A";
        case Chip::_16750:
            return "16750";
        default:
            return "Unrecognized UART chip";
        }
    }

    void initialize() {
        // Disable all interrupts.
        out8(INTERRUPT_PORT(COM1), 0x00);
        // Enable DLAB (most significant bit).
        out8(LINE_CONTROL_PORT(COM1), 0b10000000);
        // Set divisor.
        out8(DIVL_PORT(COM1), (u8)BAUD_DIVISOR);
        out8(DIVH_PORT(COM1), (u8)((u16)BAUD_DIVISOR >> 8));
        // Disable DLAB, set 8 data bits, 1 stop bit, no parity.
        out8(LINE_CONTROL_PORT(COM1), 0b00000011);
        // Enable FIFO, largest buffer possible.
        out8(FIFO_CONTROL_PORT(COM1), 0b11000111);
        u8 fifo_test = in8(INT_ID_PORT(COM1));
        if (fifo_test & (1 << 6)) {
            if (fifo_test & (1 << 7)) {
                if (fifo_test & (1 << 5))
                    chip = Chip::_16750;
                else chip = Chip::_16550A;
            }
            else chip = Chip::_16550;
        }
        else {
            u8 test_byte = 0x2a;
            out8(SCRATCH_PORT(COM1), test_byte);
            u8 scratch_returned = in8(SCRATCH_PORT(COM1));
            if (scratch_returned == test_byte)
                chip = Chip::_16450;
            else chip = Chip::_8250;
        }

#ifndef VBOX
        // Complete a loop-back test to verify all is well.
        out8(MODEM_CONTROL_PORT(COM1), 0b00011111);
        u8 test_byte = 0xae;
        out8(COM1, test_byte);
        if (in8(COM1) != test_byte) {
           Initialized = false;
           return;
        }
#endif

        // Disable loop-back, set 'Data Terminal Ready' and 'Request to Send'.
        out8(MODEM_CONTROL_PORT(COM1), 0b00001111);

        // Enable IRQs for data available interrupts.
        out8(INTERRUPT_PORT(COM1), INTERRUPT_PORT_DATA_AVAILABLE);
        Initialized = true;

        // First serial messages output from the OS.
        out("\r\n\r\nWelcome to \033[5;1;33mLensorOS\033[0m\r\n\r\n");
        out("[UART]: Initialized driver\r\n  Detected '");
        out(get_uart_chip_name(chip));
        out("' chip\r\n");

#ifdef COM1_INPUT_DEBUG
        out("[UART]: Data recieved over COM1 will be looped back in the following format:\r\n");
        out("  \"[UART]: COM1 INPUT -> <hexadecimal> <integer> <raw byte>\"");
#endif
    }

    u8 read() {
        if (Initialized == false)
            return 0;

        // TODO: We should use an actual timer instead of hardware specific "spins".
        // Wait until the UART chip flags data is ready to be read from the device.
        u16 maxSpins = (u16)1000000;
        while ((in8(LINE_STATUS_PORT(COM1)) & 0b1) == 0 && maxSpins > 0)
            maxSpins--;

        if (maxSpins == 0)
            return 0;

        u8 data = in8(DATA_PORT(COM1));
#ifdef COM1_INPUT_DEBUG
        writestr("[UART]: COM1 INPUT -> 0x");
        writestr(to_hexstring(data));
        writeb((u8)' ');
        writestr(to_string(data));
        writestr(" \033[30;47m");
        writeb(data);
        writestr("\033[0m\r\n");
#endif
        return data;
    }

    void out(u8 byte) {
        if (Initialized == false)
            return;

        // Wait for UART chip to empty it's transmit buffer before
        //   potentially over-writing data that was not yet sent.
        u16 maxSpins = (u16)1000000;
        while ((in8(LINE_STATUS_PORT(COM1)) & (1 << 5)) == 0 && maxSpins > 0)
            maxSpins--;

        if (maxSpins == 0)
            return;

        out8(COM1, byte);
    }

    void out(const char* str) {
        if (Initialized == false)
            return;

        // Set current character to beginning of string.
        char* c = (char*)str;
        // Check for null-terminator at current character.
        while (*c != '\0') {
#ifdef LENSOR_OS_UART_HIDE_COLOR_CODES
            if (*c == '\033') {
                // Loop until null terminator or 'm'.
                do { c++; } while (*c != 'm' && *c != '\0');
                // Don't read memory past null terminator!
                if (*c == '\0')
                    return;
                // Skip the 'm'.
                c++;
                if (*c == '\0')
                    return;
            }
#endif /* defined LENSOR_OS_UART_HIDE_COLOR_CODES */
            out((u8)*c);
            c++;
        }
    }

    void out(const u8* str, u64 numberOfBytes) {
        if (Initialized == false)
            return;

        while (numberOfBytes > 0) {
#ifdef LENSOR_OS_UART_HIDE_COLOR_CODES
            if (*str == '\033') {
                // Loop until no more chars or 'm'
                do {
                    str++;
                    numberOfBytes--;
                } while (*str != 'm' && numberOfBytes > 0);
                if (*str == 'm') {
                    str++;
                    numberOfBytes--;
                }
                else return;
            }
#endif /* defined LENSOR_OS_UART_HIDE_COLOR_CODES */
            out(*str);
            str++;
            numberOfBytes--;
        }
    }

    void out(u64 number) {
        out(to_string(number));
    }
    void out(u32 number) {
        out(to_string(number));
    }
    void out(u16 number) {
        out(to_string(number));
    }
}
