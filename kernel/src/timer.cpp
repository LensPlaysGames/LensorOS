#include "timer.h"

uint64_t gTicks;
uint32_t gFreq;

// Stopwatch functionality
uint64_t start;
uint64_t end;

void initialize_timer(uint32_t freq) {
	if (freq < MIN_FREQ) { freq = MIN_FREQ; }
	else if (freq > MAX_FREQ) { freq = MAX_FREQ; }
	gFreq = freq;
	uint32_t divisor = MAX_FREQ / freq;
	outb(PIT_CMD, 0b00110100);
	outb(PIT_CH0_DAT, (uint8_t) (divisor & 0x00ff));
	outb(PIT_CH0_DAT, (uint8_t)((divisor & 0xff00) >> 8));
}
