#include "pit.h"

PIT gPIT;

void PIT::initialize_pit() {
	/// Configure PIT:
	///   Channel 0
	///   H/L Bit Access
	///   Rate Generator
	///   Binary Coded Decimal Disabled
	outb(PIT_CMD, 0b00110100);
	/// Set divisor of channel zero.
	outb(PIT_CH0_DAT, (u8) ((u16)PIT_DIVISOR & 0x00ff));
	outb(PIT_CH0_DAT, (u8)(((u16)PIT_DIVISOR & 0xff00) >> 8));
}


double PIT::seconds_since_boot() {
	return (double)Ticks / PIT_FREQUENCY;
}
