#include "timer.h"

uint64_t gTicks;
uint32_t gFreq;

// Stopwatch functionality
uint64_t start;
uint64_t end;

void set_frequency(uint32_t freq) {
	if (freq < MIN_FREQ) { freq = MIN_FREQ; }
	else if (freq > MAX_FREQ) { freq = MAX_FREQ; }
	gFreq = freq;
	uint32_t divisor = MAX_FREQ / freq;
	outb(PIT_CMD, 0b00110100);
	outb(PIT_CH0_DAT, (uint8_t) (divisor & 0x00ff));
	outb(PIT_CH0_DAT, (uint8_t)((divisor & 0xff00) >> 8));
}

void initialize_timer(uint32_t freq) {
	set_frequency(freq);
}

// Get seconds elapsed for a given amount of ticks.
double get_seconds(uint64_t ticks) {
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
	uint64_t ticksToWait = seconds * gFreq;
	uint64_t startTicks = gTicks;
	while (gTicks - startTicks < ticksToWait) { asm ("hlt"); }
}

void sleep_ms(uint64_t milliseconds) {
	sleep_sec((double)milliseconds / 1000);
}
