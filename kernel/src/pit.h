#ifndef LENSOR_OS_PIT_H
#define LENSOR_OS_PIT_H

#include "integers.h"
#include "io.h"

#define PIT_MAX_FREQ 1193180
#ifdef QEMU
#define PIT_DIVISOR 59659
#else
#define PIT_DIVISOR 1193
#endif
#define PIT_FREQUENCY ((double)PIT_MAX_FREQ / PIT_DIVISOR)

#define PIT_CH0_DAT 0x40
#define PIT_CH1_DAT 0x41
#define PIT_CH2_DAT 0x42
#define PIT_CMD     0x43
#define PIT_PCSPK   0x61
// CMD BYTE BREAKDOWN
// 0b00000000
//          =   BCD/Binary (1 = BCD)
//       ===    Operating Mode
//     ==       Access Mode
//   ==         Select Channel
// Operating Mode:
//   0 0 0 =    interrupt on terminal count
//   0 0 1 =    hardware re-triggerable one-shot
//   0 1 0 =    rate generator
//   0 1 1 =    square wave generator
//   1 0 0 =    software triggered strobe
//   1 0 1 =    hardware triggered strobe
// Access Mode:
//   0 0 =      latch count value
//   0 1 =      low only
//   1 0 =      high only
//   1 1 =      low/high
// Select Channel:
//   0 0 =      channel 0
//   0 1 =      channel 1
//   1 0 =      channel 2
//   1 1 =      read-back command (8254 only)

class Scheduler;

class PIT {
    friend class Scheduler;

    enum Channel {
        Zero = 0,
        One  = 1,
        Two  = 2,
    };

    enum Access {
        LatchCount = 0b00,
        LowOnly    = 0b01,
        HighOnly   = 0b10,
        HighAndLow = 0b11,
    };

    enum Mode {
        InterruptOnTerminalCount     = 0b000,
        HardwareRetriggerableOneShot = 0b001,
        RateGenerator                = 0b010,
        SquareWaveGenerator          = 0b011,
        SoftwareStrobe               = 0b100,
        HardwareStrobe               = 0b101,
    };

public:
    PIT();

    void tick() { Ticks += 1; }

    u64 get() { return Ticks; }
    double seconds_since_boot();

    /* PIT Channel two is connected to PC Speaker when
     *   bit 0 of I/O port 0x61 is equal to one.
     */
    void play_sound(u64 freq, double duration);

private:
    u64 Ticks { 0 };

    void configure_channel(Channel, Access, Mode, u64 freq);

    void start_speaker();
    void stop_speaker();
};

extern PIT gPIT;

#endif /* LENSOR_OS_PIT_H */
