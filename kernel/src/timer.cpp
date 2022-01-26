#include "timer.h"

u64 gTicks;
u32 gFreq;

// Stopwatch functionality
u64 start;
u64 end;

void set_frequency(u32 freq) {
	if (freq < MIN_FREQ) { freq = MIN_FREQ; }
	else if (freq > MAX_FREQ) { freq = MAX_FREQ; }
	gFreq = freq;
	u32 divisor = MAX_FREQ / freq;
	outb(PIT_CMD, 0b00110100);
	outb(PIT_CH0_DAT, (u8) (divisor & 0x00ff));
	outb(PIT_CH0_DAT, (u8)((divisor & 0xff00) >> 8));
}

void initialize_timer(u32 freq) {
	set_frequency(freq);
}

// Get seconds elapsed for a given amount of ticks.
double get_seconds(u64 ticks) {
	return ticks / (double)gFreq;
}

double get_seconds() {
	return get_seconds(gTicks);
}

double timer_elapsed_seconds() {
	return get_seconds(end - start);
}

void sleep_sec(double seconds) {
	// Calculate number of ticks needed to wait.
	u64 ticksToWait = seconds * gFreq;
	u64 startTicks = gTicks;
	while (gTicks - startTicks < ticksToWait) { asm ("hlt"); }
}

void sleep_ms(u64 milliseconds) {
	sleep_sec((double)milliseconds / 1000);
}
