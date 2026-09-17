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

#include <bits/decls.h>
#include <integers.h>
#include <time.h>
#include <x86_64/io.h>
#include <x86_64/pit.h>

PIT gPIT;
void pit_tick() { gPIT.tick(); }

PIT::PIT() {
    configure_channel(
        Channel::Zero,
        Access::HighAndLow,
        Mode::RateGenerator,
        PIT_FREQUENCY);
    Frequency = PIT_FREQUENCY;
    configure_channel(
        Channel::Two,
        Access::HighAndLow,
        Mode::SquareWaveGenerator,
        440);
}

usz PIT::seconds_since_boot() {
    return (usz)Ticks / Frequency;
}
usz PIT::milliseconds_since_boot() {
    return (usz)Ticks * 1000 / Frequency;
}

void PIT::prepare_wait_milliseconds(usz ms) {
    TicksToWait = ms * Frequency / Time::milliseconds_per_second;
    // std::print("[PIT]: Prepared wait for {} ticks\n", TicksToWait);
}

void PIT::wait() {
    u64 tickToWaitTo = Ticks + TicksToWait;
    // std::print(
    //     "[PIT]: Waiting for {} ticks, current is {}, deadline is {}\n",
    //     TicksToWait,
    //     (size_t)Ticks,
    //     tickToWaitTo);
    while (Ticks < tickToWaitTo)
        asm volatile("pause" ::: "memory");
}

void PIT::wait_polling(usz milliseconds) {
    auto frequency = Time::milliseconds_per_second / milliseconds;
    // std::print("wait_polling({}ms): freq={}hz\n", milliseconds, frequency);

    // Ensure channel two can count...
    enable_speaker();

    // Configure channel two for a one shot count down
    configure_channel(
        Channel::Two,
        Access::HighAndLow,
        Mode::RateGenerator,
        frequency);

    uint16_t last_count = 0xffff;
    while (true) {
        // Channel 2 Counter Latch Command
        out8(PIT_CMD, 0x80);
        uint8_t lsb = in8(PIT_CH2_DAT);
        uint8_t msb = in8(PIT_CH2_DAT);
        uint16_t current_count = (((uint16_t)msb) << 8) | lsb;

        // If the count jumps back up or passes zero, our window is finished
        if (current_count > last_count)
            break;

        last_count = current_count;
        asm volatile("pause" ::: "memory");
    }
}

void PIT::enable_speaker() {
    u8 tmp = in8(PIT_PCSPK);
    tmp &= ~0b11;
    tmp |= 1;
    out8(PIT_PCSPK, tmp);
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

void PIT::play_sound(u64 frequency, usz ms) {
    if (frequency == 0 or ms == 0) return;

    enable_speaker();
    configure_channel(
        Channel::Two,
        Access::HighAndLow,
        Mode::SquareWaveGenerator,
        frequency);

    prepare_wait_milliseconds(ms);

    start_speaker();
    asm volatile("lfence" ::: "memory");
    wait();
    asm volatile("lfence" ::: "memory");
    stop_speaker();
}

void PIT::configure_channel(Channel channel, Access access, Mode mode, u64 frequency) {
    if (access == Access::LatchCount)
        out8(PIT_CMD, channel);

    // Interrupt on Terminal Count mode only works on channel zero.
    if (channel != Channel::Zero and mode == Mode::InterruptOnTerminalCount)
        return;

    // Input gate can't be changed in channels zero or one, and hardware strobe relies on this.
    if (mode == Mode::HardwareStrobe and channel == Channel::Zero)
        return;

    // Divisor must not be `1` in these modes.
    if (
        (mode == Mode::RateGenerator or mode == Mode::SquareWaveGenerator)
        and frequency == PIT_MAX_FREQ)
        return;

    if (frequency == 0)
        return;

    _PushIgnoreWarning("-Wdeprecated-enum-enum-conversion");
    u8 command = (channel | access | mode) & ~1;
    _PopWarnings();

    u16 dataPort = PIT_CH0_DAT;
    u16 divisor = PIT_MAX_FREQ / frequency;
    // std::print("[PIT]: freq={} divisor={}\n", frequency, divisor);

    out8(PIT_CMD, command);

    if (channel == Channel::Two)
        dataPort = PIT_CH2_DAT;
    if (access == Access::HighAndLow or access == Access::HighOnly)
        out8(dataPort, (u8)(divisor & 0x00ff));
    if (access == Access::HighAndLow or access == Access::LowOnly)
        out8(dataPort, (u8)((divisor & 0xff00) >> 8));
}
