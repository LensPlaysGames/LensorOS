#ifndef LENSOR_OS_TIMER_H
#define LENSOR_OS_TIMER_H

#include <stdint.h>
#include "io.h"

#define MIN_FREQ 20
#define MAX_FREQ 1193180

#define PIT_CH0_DAT 0x40
#define PIT_CH1_DAT 0x41
#define PIT_CH2_DAT 0x42
#define PIT_CMD     0x43
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

void initialize_timer(uint32_t freq);

double get_seconds(uint64_t startTick = 0);

extern uint64_t gTicks;
extern uint32_t gFreq;

#endif
