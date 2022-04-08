#ifndef LENSOR_OS_UART_H
#define LENSOR_OS_UART_H

#include <io.h>
#include <integers.h>

#define BAUD_FREQ 115200
#define BAUD_RATE 9600
#define BAUD_DIVISOR (BAUD_FREQ / BAUD_RATE)

#define COM1 0x3f8
#define COM2 0x2f8

#define DATA_PORT(base)           (base + 0)
#define RECIEVE_BUFFER_PORT(base) (base + 0)
#define DIVL_PORT(base)           (base + 0)
#define DIVH_PORT(base)           (base + 1)
#define INTERRUPT_PORT(base)      (base + 1)
#define INT_ID_PORT(base)         (base + 2)
#define FIFO_CONTROL_PORT(base)   (base + 2)
#define LINE_CONTROL_PORT(base)   (base + 3)
#define MODEM_CONTROL_PORT(base)  (base + 4)
#define LINE_STATUS_PORT(base)    (base + 5)
#define MODEM_STATUS_PORT(base)   (base + 6)
#define SCRATCH_PORT(base)        (base + 7)

#define INTERRUPT_PORT_DATA_AVAILABLE 1
#define INTERRUPT_PORT_TRANSMITTER_HOLDING_REGISTER_EMPTY (1 << 1)
#define INTERRUPT_PORT_LINE_STATUS_CHANGED                (1 << 2)
#define INTERRUPT_PORT_MODEM_STATUS_CHANGED               (1 << 3)

// Uncomment the following preprocessor directive to print the
//   input recieved in COM1 back out to COM1 in the following format.
// "[UART]: COM1 INPUT -> <hexadecimal> <integer> <raw byte>"
// #define COM1_INPUT_DEBUG

/* Port Register Offsets ([PORT] + [OFFSET])
 * 0: Data
 *      When DLAB (Divisor Latch Access Bit) is 1, Least Significant Byte of Divisor.
 * 1: Interrupt Enable
 *      When DLAB is 1, Most Significant Byte of Divisor
 *      Bit 0: Recieved Data Available
 *               Incoming data is now available within the buffer.
 *          1: Transmitter Holding Register Empty
 *               Output buffer is empty and data transmission can be completed.
 *          2: Break/Error
 *          3: On Status Changed
 *          4-7: Unused
 *      16750 ONLY:
 *        Bit 4: Sleep Mode
 *            5: Low Power Mode
 * 2: Interrupt ID (read-only)
 *      Bit 0: Interrupt Pending Flag
 *          1-3: Type of interrupt
 *            000  --  Modem Status
 *            001  --  Transmitter Holding Register Empty
 *            010  --  Data Available
 *            011  --  Receiver Line Status Changed
 *            100  --  Reserved
 *            101  --  Reserved
 *            110  --  Time-out Interrupt Pending (16550 & later)
 *            111  --  Reserved
 *          4-5: Reserved
 *          6-7: FIFO Info
 *            00  --  No FIFO on chip
 *            01  --  Reserved
 *            01  --  FIFO Enabled, Doesn't Work
 *            11  --  FIFO Enabled
 *      16750 ONLY:
 *        Bit 5: 64 Byte FIFO Enabled
 * 2: FIFO Control (write-only)
 *      Bit 0: Enable FIFOs
 *          1: Clear Recieve FIFOs
 *          2: Clear Transmit FIFOs
 *          3: DMA Mode Select
 *          4-5: Reserved
 *          6-7: FIFO Control (buffer size)
 *            00  --  1 Byte    (1 Byte)
 *            01  --  4 Bytes   (16 Bytes)
 *            10  --  8 Bytes   (32 Bytes)
 *            11  --  14 Bytes  (56 Bytes)
 *      16750 ONLY:
 *        Bit 5: Enable 64 Byte FIFO
 * 3: Line Control (Most Significan Bit is DLAB)
 *      Bits 0-1: Number of Data Bits
 *        0 0 = 5,  0 1 = 6,  1 0 = 7,  1 1 = 8
 *      2: Stop Bits
 *        0 = 1,  1 = 1.5/2 (depends on data bits)
 *      3-5: Parity
 *        0 0 0 = NONE, 0 0 1 = ODD, 0 1 1 = EVEN, 1 0 1 = MARK, 1 1 1 = SPACE  
 * 4: Modem Control
 *      Bit 0: Data Terminal Ready
 *          1: Request to Send
 *          2: Auxiliary Output 1
 *          3: Auxiliary Output 2
 *          4: Loopback Mode
 *          5-7: Unused
 *      16750 ONLY:
 *        Bit 5: Autoflow Control Enabled
 * 5: Line Status
 *      Bit 0: Data Ready
 *          1: Overrun Error
 *          2: Parity Error
 *          3: Framing Error
 *          4: Break Indicator
 *          5: Capable of Data Reception
 *          6: Transmit Buffer Empty, Shift Register Done
 *          6: Empty Data Holding Registers
 *          7: Error in Received FIFO
 * 6: Modem Status
 *      Bit 0: Delta Clear to Send
 *          1: Delta Data Set Ready
 *          2: Trailing Edge of Ring Indicator
 *          3: Deltra Data Carrier Detect
 *          4: Clear to Send
 *          5: Data Set Ready
 *          6: Ring Indicator
 *          7: Data Carrier Detect
 * 7: Scratch
 *      On newer UART chips (>8250x), this register will relay whatever
 *        was last written to it when read from.
 */

// TODO: Add capability for selecting communication channel (COM1, COM2, etc).
namespace UART {
    void initialize();
    
    enum class Chip;
    const char* get_uart_chip_name(Chip);

    // Called from the UART interrupt handler when data is received.
    u8 read();

    // Write a singular byte verbatim to serial output.
    void out(u8);
    inline void outc(char c) {
        out((u8)c);
    }

    // Write a c-style null-terminated string to serial output.
    void out(const char* string);
    // Write a number of bytes from a given buffer to serial output.
    void out(u8* buffer, u64 numberOfBytes);
    // Write the given number as a string to serial output.
    void out(u64);
    void out(u32);
    void out(u16);
}

#endif
