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

#include <pit.h>

#include <integers.h>
#include <io.h>

PIT gPIT;
void pit_tick() { gPIT.tick(); }

PIT::PIT() {
    configure_channel(Channel::Zero, Access::HighAndLow, Mode::RateGenerator, PIT_FREQUENCY);
    configure_channel(Channel::Two, Access::HighAndLow, Mode::SquareWaveGenerator, 440);
}

double PIT::seconds_since_boot() {
    return (double)Ticks / PIT_FREQUENCY;
}

void PIT::prepare_wait_seconds(double duration) {
    TicksToWait = duration * PIT_FREQUENCY;
}

void PIT::wait() {
    u64 tickToWaitTo = Ticks + TicksToWait;
    while (Ticks < tickToWaitTo)
        asm volatile ("hlt");
}

void PIT::start_speaker() {
    u8 tmp = in8(PIT_PCSPK);
    tmp |= 0b11;
    out8(PIT_PCSPK, tmp);
}

void PIT::stop_speaker() {
    u8 tmp = in8(PIT_PCSPK);
    tmp &= 0b11111100;
    out8(PIT_PCSPK, tmp);
}

void PIT::play_sound(u64 frequency, double duration) {
    if (frequency == 0 || duration <= 0)
        return;

    configure_channel(Channel::Two, Access::HighAndLow, Mode::SquareWaveGenerator, frequency);

    // FIXME: Playing a sound shouldn't block the entire system :^)
    //   I should probably create a separate process that runs sound.
    prepare_wait_seconds(duration);
    start_speaker();
    wait();
    stop_speaker();
}

void PIT::configure_channel(Channel channel, Access access, Mode mode, u64 frequency) {
    if (access == Access::LatchCount)
        out8(PIT_CMD, channel);
    // Interrupt on Terminal Count mode only works on channel zero.
    if (channel != Channel::Zero && mode == Mode::InterruptOnTerminalCount)
        return;
    // Input gate can't be changed in channels zero or one, and hardware strobe relies on this.
    if (mode == Mode::HardwareStrobe && channel == Channel::Zero)
        return;
    // Divisor must not be `1` in these modes.
    if ((mode == Mode::RateGenerator || mode == Mode::SquareWaveGenerator) && frequency == PIT_MAX_FREQ)
        return;
    if (frequency == 0)
        return;

    _PushIgnoreWarning("-Wdeprecated-enum-enum-conversion")
    u8 command = (channel | access | mode) & ~1;
    _PopWarnings()
    u16 dataPort = PIT_CH0_DAT;
    u16 divisor = PIT_MAX_FREQ / frequency;

    out8(PIT_CMD, command);

    if (channel == Channel::Two)
        dataPort = PIT_CH2_DAT;
    if (access == Access::HighAndLow || access == Access::HighOnly)
        out8(dataPort, (u8)(divisor & 0x00ff));
    if (access == Access::HighAndLow || access == Access::LowOnly)
        out8(dataPort, (u8)((divisor & 0xff00) >> 8));
}
