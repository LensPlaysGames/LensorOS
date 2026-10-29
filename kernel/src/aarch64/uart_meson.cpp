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

volatile uint8_t* wfifo() { return (uint8_t*)(Base + 0); }
volatile uint8_t* rfifo() { return (uint8_t*)(Base + 4); }
volatile uint32_t* control() { return (uint32_t*)(Base + 8); }
volatile uint32_t* status() { return (uint32_t*)(Base + 12); }
volatile uint32_t* misc() { return (uint32_t*)(Base + 16); }
volatile uint32_t* reg5() { return (uint32_t*)(Base + 20); }

// Called from the UART interrupt handler when data is received.
uint8_t read() {
    if (not initialized()) return 0;

    uint maxSpins = 1000000u;
    // bit 20 -- receive FIFO empty
    while ((*status() & (1u << 20)) and maxSpins > 0)
        maxSpins--;

    if (maxSpins == 0)
        return 0;

    return *rfifo();
}

void out_raw(uint8_t byte) {
    if (initialized() == false)
        return;

    // Wait for UART chip to empty it's transmit buffer before potentially
    // over-writing data that was not yet sent, or causing a "FIFO written
    // while full" error.
    uint maxSpins = 1000000u;
    // bit 21 -- transmit FIFO full
    while ((*status() & (1u << 21)) and maxSpins > 0)
        maxSpins--;

    if (maxSpins == 0)
        return;

    *wfifo() = byte;
}

}  // namespace UART

/**
 ** The following register definition is uniformly applied to all UART
 ** instantiations in the chip.
 **
 ** Instantiations   Base Address
 ** UART0            0xc1100000 + (0x2130 ~ 0x2135) * 4
 ** UART1            0xc1100000 + (0x2137 ~ 0x213c) * 4
 ** UART2            0xc1100000 + (0x21C0 ~ 0x21c5) * 4
 ** UART0-AO         0xc81004c0 ~ 0xc81004d4
 ** UART2-AO         0xc81004e0 ~ 0xc81004f4
 **
 ** UARTx_WFIFO: Write data
 ** [31:8] Read-only default=0 -- UNUSED
 **  [7:0] Read+write -- Write FIFO data. The Write FIFO holds 64 bytes. The
 **        Write FIFO can be written as long as it is not full.
 **
 ** UARTx_RFIFO: Read Data
 ** [31:8] Read-only default=0 -- UNUSED
 **  [7:0] Read+write -- Read FIFO data. The Read FIFO holds 64 bytes. The empty
 **        flag can be used to determine if data is available
 **
 ** UARTx_CONTROL: UART Mode
 ** 31      Read+write default=0 -- Invert the RTS signal
 ** 30      Read+write default=0 -- Mask Error: Set to 1 to mask errors
 ** 29      Read+write default=0 -- Invert the CTS signal
 ** 28      Read+write default=0 -- Transmit byte Interrupt: Set to 1 to
 **         enable the generation an interrupt whenever a byte is read from the
 **         transmit FIFO
 ** 27      Read+write default=0 -- Receive byte Interrupt: Set to 1 to
 **         enable the generation an interrupt whenever a byte is written to the
 **         receive FIFO
 ** 26      Read+write default=0 -- Set to 1 to invert the TX pin
 ** 25      Read+write default=0 -- Set to 1 to invert the RX pin
 ** 24      Read+write default=0 -- Clear Error
 ** 23      Read+write default=0 -- Reset the receive state machine
 ** 22      Read+write default=0 -- Reset the transmit state machine
 ** [21:20] Read+write default=0 -- Character length
 **         00 = 8 Bits, 01 = 7 Bits,
 **         10 = 6 Bits, 11 = 5 Bits
 ** 19      Read+write default=1 -- Parity Enable: Set to 1 to enable parity
 ** 18      Read+write default=0 -- Parity type: 0 = even, 1 = odd
 ** [17:16] Read+write default=0 -- Stop bit length: 00 = 1 bit, 01 = 2 Bits
 ** 15      Read+write default=0 -- Two Wire mode:
 ** 14      Read+write default=0 -- Unused
 ** 13      Read+write default=0 -- Receive Enable. Set to 1 to enable the UART receive function
 ** 12      Read+write default=0 -- Transmit Enable. Set to 1 to enable the UART transmit function
 ** [11:0]  Read+write default=0x120 -- Baud rate: This value sets the baud rate by dividing the MPEG system clock.
 **
 ** UARTx_STATUS: UART Status
 ** [31:27] Read-only default=0 -- Unused
 **      26 Read-only default=0 -- UART_RECV_BUSY: This bit will be 1 if the
 **         uart receive state machine is busy
 **      25 Read-only default=0 -- UART_XMIT_BUSY: This bit will be 1 if the
 **         uart transmit state machine is busy
 **      24 Read-only default=0 -- RECV_FIFO_OVERFLOW:
 **      23 Read-only default=0 -- CTS Level
 **      22 Read-only default=0 -- Transmit FIFO Empty
 **      21 Read-only default=0 -- Transmit FIFO Full
 **      20 Read-only default=0 -- Receive FIFO empty
 **      19 Read-only default=0 -- Receive FIFO full
 **      18 Read-only default=0 -- This bit is set if the FIFO is written when
 **         it is full.
 **         To clear this bit, write bit 24 of UARTx_CONTROL register.
 **      17 Read-only default=0 -- Frame error.
 **         To clear this bit, write bit 24 of UARTx_CONTROL register.
 **      16 Read-only default=0 -- Parity error.
 **         To clear this bit, write bit 24 of UARTx_CONTROL register.
 **      15 Read-only default=0 -- Unused
 **  [14-8] Read-only default=0 -- Transmit FIFO count. Number of bytes in
 **         the transmit FIFO
 **       7 Read-only default=0 -- Unused
 **   [8:0] Read-only default=0 -- Receive FIFO count. Number of bytes in the
 **   receive FIFO
 **
 ** UARTx_MISC: UART IRQ CONTROL
 ** 31 R/W 0 Added a “just in case” bit that can be set to 1 to enable clocks always. The default is 0 meaning the auto-clock
 ** gating logic is enabled.
 ** 30 R/W 0 USE old Rx Baud: There was a bug in the RX baud rate generator. The Rx baud rate generator was re-designed
 ** to compute a baud rate correctly. If you want to use the old (stupid) logic, you can set This bit to 1.
 ** 29 R/W 0 ASYNC_FIFO_PURGE: This bit can be set to 1 after all UART bytes have been received in order to purge the data
 ** into the Async FIFO. This bit is needed because the UART receives 8-bit data, but the ASYNC FIFO can only be
 ** written with 16-bit data. In this case there might be a residual byte if the UART is not receiving an even number
 ** of bytes.
 ** 28 R/W 0 ASYNC_FIFO_EN: If This bit is set to 1, then the UART received data is automatically sent to the Async FIFO module
 ** which will in turn automatically send the data to DDR memory
 ** 27 R/W 0 CTS: Filter Timebase select: 1 = 1uS, 0 = 111nS timebase. A filter was added to the CTS input to allow for a little
 ** digital filtering. The amount of filtering is controlled by this timebase (longer = more filtering) and the value in
 ** Bits FILTER_SEL below.
 ** 26-24 R/W 0 CTS: FILTER_SEL: 0 = no filter, 7 = max filtering
 ** 23-20 R/W 0 BAUD_RATE_EXT: These 4 Bits extend the baud rate divider to 16-bits:
 ** Baud Rate = {Reg4[23:20],Reg2[11:0]}
 ** 19 R/W 0 RX: Filter Timebase select: 1 = 1uS, 0 = 111nS timebase. A filter was added to the RX input to allow for a little
 ** digital filtering. The amount of filtering is controlled by this timebase (longer = more filtering) and the value in
 ** Bits FILTER_SEL below.
 ** 18-16 R/W 0 RX: FILTER_SEL: 0 = no filter, 7 = max filtering
 ** 15-8 R/W 32 XMIT_IRQ_CNT: The UART can be configured to generate an interrupt if the number of bytes in the transmit FIFO
 ** drops below this value.
 ** 7:0 R/W 15 RECV_IRQ_CNT: The UART can be configured to generate an interrupt after a certain number of bytes have been
 ** received by the UART.
 **
 ** UARTx_REG5
 ** 31-24 R/W 0 unused
 ** 24 R/W 0 USE_XTAL_CLK: If This bit is set, then the clock for generating the UART Baud rate comes from the crystal pad.
 ** This allows the UART to operate independent of clk81.
 ** 23 R/W 0 USE New Baud rate. Over the years, the baud rate has been extended by concatenating Bits from different
 ** registers. To take advantage of the full 23-bit baud rate generate (extended to 23 Bits to accommodate very low
 ** baud rates), you must set This bit. If This bit is set, then the baud rate is configured using Bits [22:0] below
 ** 22:0 R/W 15 NEW_BAUD_RATE: If Bit[23] = 1 above, then the baud rate for the UART is computed using these Bits.
 **/
