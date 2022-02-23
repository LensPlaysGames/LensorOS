#ifndef LENSOR_OS_IO_H
#define LENSOR_OS_IO_H

#include "integers.h"

void out8  (u16 port, u8 value);
u8   in8   (u16 port);
void out16 (u16 port, u16 value);
u16  in16  (u16 port);
void out32 (u16 port, u32 value);
u32  in32  (u16 port);

/* By writing to a port that is known to be unused,
 *   it is possible to 'delay' the CPU by a microsecond or two.
 * This is useful for 'slow' hardware (ie. RTC) that needs some
 *   time for the serial operation to take effect.
 * Port 0x80 is used by BIOS for POST codes, and reading from/writing to
 *   it is effectively guaranteed to not adversely affect the hardware.
 */
void io_wait();

#endif
