#include "uart.h"

#include "cstr.h"
#include "memory/heap.h"

// Global serial driver.
UARTDriver* srl = nullptr;

const char* get_uart_chip_name(UARTChip chip) {
    switch (chip) {
    case UARTChip::NONE:
        return "Invalid UART chip";
    case UARTChip::_8250:
        return "8250";
    case UARTChip::_16450:
        return "16450";
    case UARTChip::_16550:
        return "16550";
    case UARTChip::_16550A:
        return "16550A";
    case UARTChip::_16750:
        return "16750";
    default:
        return "Unrecognized UART chip";
    }
}

UARTDriver::UARTDriver() {
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
                chip = UARTChip::_16750;
            else chip = UARTChip::_16550A;
        }
        else chip = UARTChip::_16550;
    }
    else {
        u8 test_byte = 0x2a;
        out8(SCRATCH_PORT(COM1), test_byte);
        u8 scratch_returned = in8(SCRATCH_PORT(COM1));
        if (scratch_returned == test_byte)
            chip = UARTChip::_16450;
        else chip = UARTChip::_8250;
    }
    // Loop-back test.
    out8(MODEM_CONTROL_PORT(COM1), 0b00011111);
    u8 test_byte = 0xae;
    out8(COM1, test_byte);
    if (in8(COM1) != test_byte) {
        // Error! No good.
        // TODO: Something about it.
    }
    // Disable loop-back, set 'Data Terminal Ready' and 'Request to Send'.
    out8(MODEM_CONTROL_PORT(COM1), 0b00001111);

    // First serial messages output from the OS.
    writestr("\r\n\r\nWelcome to \033[5;1;33mLensorOS\033[0m\r\n\r\n");
    writestr("[UART]: Initialized driver\r\n  Detected '");
    writestr(get_uart_chip_name(chip));
    writestr("' chip\r\n");

#ifdef COM1_INPUT_DEBUG
    writestr("[UART]: Data recieved over COM1 will be looped back in the following format:\r\n");
    writestr("  \"[UART]: COM1 INPUT -> <hexadecimal> <integer> <raw byte>\"");
#endif

    // Enable IRQs for data available interrupts.
    out8(INTERRUPT_PORT(COM1), INTERRUPT_PORT_DATA_AVAILABLE);
}

/// Read a byte of data over the serial communications port COM1.
u8 UARTDriver::readb() {
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

/// Write a byte of data over the serial communications port COM1.
void UARTDriver::writeb(u8 data) {
    // Wait for UART chip to empty it's transmit buffer before
    //   potentially over-writing data that was not yet sent.
    u16 maxSpins = (u16)1000000;
    while ((in8(LINE_STATUS_PORT(COM1)) & (1 << 5)) == 0 && maxSpins > 0)
        maxSpins--;
    if (maxSpins == 0) return;

    out8(COM1, data);
}

/// Write a C-style null-terminated byte-string to the serial output COM1.
void UARTDriver::writestr(const char* str) {
    // Set current character to beginning of string.
    char* c = (char*)str;
    // Check for null-terminator at current character.
    while (*c != 0) {
#ifdef LENSOR_OS_UART_HIDE_COLOR_CODES
        if (*c == '\33' || *c == '\033' || *c == '\x1b' || *c == '\x1B') {
            // Loop until null terminator or 'm'.
            do { c++; } while (*c != 'm' && *c != 0);
            // Don't read memory past null terminator!
            if (*c == 0)
                return;
            // Skip the 'm'.
            c++;
            if (*c == 0)
                return;
        }
#endif
        writeb((u8)*c);
        c++;
    }
}

/// Write a given number of characters from a given string of characters to serial output COM1.
void UARTDriver::writestr(char* str, u64 numChars) {
    while (numChars > 0) {
#ifdef LENSOR_OS_UART_HIDE_COLOR_CODES
        if (*str == '\33' || *str == '\033' || *str == '\x1b' || *str == '\x1B') {
            // Loop until no more chars or 'm'
            do {
                str++;
                numChars--;
            } while (*str != 'm' && numChars > 0);
            if (*str == 'm') {
                str++;
                numChars--;
            }
            if (numChars == 0)
                return;
        }
#endif
        writeb((u8)*str);
        str++;
        numChars--;    
    }
}

void UARTDriver::writestr(u64 number) {
    writestr(to_string(number));
}
