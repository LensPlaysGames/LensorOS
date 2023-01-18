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

#ifndef LENSOR_OS_PIT_H
#define LENSOR_OS_PIT_H

#include <integers.h>
#include <io.h>

#define PIT_MAX_FREQ 1193180
#if defined QEMU || defined VBOX || defined VMWARE
#define PIT_DIVISOR 59659
#else
#define PIT_DIVISOR 1193
#endif /* defined QEMU || defined VBOX || defined VMWARE */
#define PIT_FREQUENCY ((double)PIT_MAX_FREQ / PIT_DIVISOR)

#define PIT_CH0_DAT 0x40
#define PIT_CH1_DAT 0x41
#define PIT_CH2_DAT 0x42
#define PIT_CMD     0x43
#define PIT_PCSPK   0x61
/* CMD BYTE BREAKDOWN
 *   0b00000000
 *            =   BCD/Binary (1 = BCD)
 *         ===    Operating Mode
 *       ==       Access Mode
 *     ==         Select Channel
 *   Operating Mode:
 *     0 0 0 =    interrupt on terminal count
 *     0 0 1 =    hardware re-triggerable one-shot
 *     0 1 0 =    rate generator
 *     0 1 1 =    square wave generator
 *     1 0 0 =    software triggered strobe
 *     1 0 1 =    hardware triggered strobe
 *   Access Mode:
 *     0 0 =      latch count value
 *     0 1 =      low only
 *     1 0 =      high only
 *     1 1 =      low/high
 *   Select Channel:
 *     0 0 =      channel 0
 *     0 1 =      channel 1
 *     1 0 =      channel 2
 *     1 1 =      read-back command (8254 only)
*/

/* TODO:
 * |- Lock the PIT when it's being used so
 * |    threads don't stomp on each other.
 * `- Implement Read-Back command support.
 */
class PIT {
    // IO Port of PIT channel
    // Channel One is not guaranteed to be
    //   implemented, especially on modern hardware.
    enum Channel {
        Zero = 0b00000000,
        Two  = 0b10000000,
    };

    enum Access {
        LatchCount = 0b00000000,
        LowOnly    = 0b00010000,
        HighOnly   = 0b00100000,
        HighAndLow = 0b00110000,
    };

    enum Mode {
        InterruptOnTerminalCount     = 0b00000000,
        HardwareRetriggerableOneShot = 0b00000010,
        RateGenerator                = 0b00000100,
        SquareWaveGenerator          = 0b00000110,
        SoftwareStrobe               = 0b00001000,
        HardwareStrobe               = 0b00001010,
    };

public:
    PIT();

    void tick() { Ticks += 1; }

    u64 get() { return Ticks; }
    //double seconds_since_boot();

    /* PIT Channel two is connected to PC Speaker when
     *   bit 0 of I/O port 0x61 is equal to one.
     */
    void play_sound(u64 freq, double duration);

    /// Prepare an amount of time to wait.
    void prepare_wait_seconds(double);

    /// Wait for the prepared amount of time.
    void wait();

private:
    /// Incremented by IRQ0 interrupt handler.
    volatile u64 Ticks { 0 };
    /* `wait()` stops spinning as soon `Ticks` reaches
     *   offset from `Ticks` at beginning of spinning.
     */
    u64 TicksToWait { 0 };

    void configure_channel(Channel, Access, Mode, u64 freq);

    /* Playing sound out of the PC Speaker by
     *   manipulating bits 0 & 1 of IO port 0x61.
     */
    void start_speaker();
    void stop_speaker();
};

extern PIT gPIT;

// This work-around/hack is due to needing a function pointer 
void pit_tick();

#endif /* LENSOR_OS_PIT_H */
