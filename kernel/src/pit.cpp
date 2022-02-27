#include "pit.h"

#include "cstr.h"
#include "integers.h"
#include "io.h"
#include "uart.h"

PIT gPIT;

PIT::PIT() {
    configure_channel(Channel::Zero, Access::HighAndLow, Mode::RateGenerator, PIT_FREQUENCY);
    configure_channel(Channel::Two, Access::HighAndLow, Mode::SquareWaveGenerator, 440);
}

double PIT::seconds_since_boot() {
    return (double)Ticks / PIT_FREQUENCY;
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
    configure_channel(Channel::Two, Access::HighAndLow, Mode::SquareWaveGenerator, frequency);

    u64 ticksToWait = duration * PIT_FREQUENCY;

    u64 tickToWaitTo = Ticks + ticksToWait;
    start_speaker();
    // FIXME: Playing a sound shouldn't block the entire system :^)
    //   I should probably create a separate process that runs sound.
    while (Ticks < tickToWaitTo)
        asm volatile ("nop");
    
    stop_speaker();
}

void PIT::configure_channel(Channel channel, Access access, Mode modeOfOperation, u64 frequency) {
    u8 channelBits = channel << 6;
    u8 accessBits = access << 4;
    u8 modeBits = modeOfOperation << 1;
    u8 command = (channelBits | accessBits | modeBits) & ~1;
    out8(PIT_CMD, command);

    u16 dataPort { 0 };
    switch (channel) {
    case Channel::Zero:
        dataPort = PIT_CH0_DAT;
        break;
    case Channel::One:
        dataPort = PIT_CH1_DAT;
        break;
    case Channel::Two:
        dataPort = PIT_CH2_DAT;
        break;
    default:
        return;
    }

    u16 divisor = PIT_MAX_FREQ / frequency;
    out8(dataPort, (u8)(divisor & 0x00ff));
    out8(dataPort, (u8)((divisor & 0xff00) >> 8));
}
