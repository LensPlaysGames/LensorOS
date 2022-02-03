#ifndef LENSOR_OS_UART_H
#define LENSOR_OS_UART_H

#include "io.h"
#include "integers.h"

#define BAUD_FREQ 115200
#define BAUD_RATE 115200
#define BAUD_DIVISOR (BAUD_FREQ / BAUD_RATE)

#define MAX_SERIAL_STRING_LENGTH 1024

#define COM1 0x3f8

/*
Port Register Offsets ([PORT] + [OFFSET])
0: Data
     When DLAB (Divisor Latch Access Bit) is 1, Least Significant Byte of Divisor
1: Interrupt Enable
     Bit 0: Data Available
         1: Transmitter Empty
         2: Break/Error
         3: Status Change
         4-7: Unused
     When DLAB is 1, Most Significant Byte of Divisor
2: Interrupt ID & FIFO Control
3: Line Control (Most Significan Bit is DLAB)
     Bits 0-1: Number of Data Bits
       0 0 = 5,  0 1 = 6,  1 0 = 7,  1 1 = 8
     2: Stop Bits
       0 = 1,  1 = 1.5/2 (depends on data bits)
     3-5: Parity
       0 0 0 = NONE, 0 0 1 = ODD, 0 1 1 = EVEN, 1 0 1 = MARK, 1 1 1 = SPACE  
4: Modem Control
     Bit 0: Data Terminal Ready
         1: Request to Send
         2: Out 1
         3: Out 2
         4: Loop
         5-7: Unused
5: Line Status
     Bit 0: Data Ready
         1: Overrun Error
         2: Parity Error
         3: Framing Error
         4: Break Indicator
         5: Transmitter Holding Register Empty
         6: Trasmitter Empty
         7: Impending Error
6: Modem Status
     Bit 0: Delta Clear to Send
         1: Delta Data Set Ready
         2: Trailing Edge of Ring Indicator
         3: Deltra Data Carrier Detect
         4: Clear to Send
         5: Data Set Ready
         6: Ring Indicator
         7: Data Carrier Detect
7: Scratch
*/

// TODO: Add capability for selecting communication channel (COM1, COM2, etc).
class UARTDriver {
public:
    UARTDriver();

    u8 readb();
    void writeb(u8 data);

    /// Write a C-style null-terminated byte-string to the serial output COM1.
    void writestr(const char* str);
    void writestr(char* str, u64 numChars);
};

extern UARTDriver srl;

#endif
