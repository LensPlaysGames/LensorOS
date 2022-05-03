#ifndef LENSOR_OS_PANIC_H
#define LENSOR_OS_PANIC_H

#define PanicStartX 400
#define PanicStartY 500

class InterruptFrame;
class InterruptFrameError;

__attribute__((no_caller_saved_registers))
void panic(const char* panicMessage);

__attribute__((no_caller_saved_registers))
void panic(InterruptFrame*, const char* panicMessage);

__attribute__((no_caller_saved_registers))
void panic(InterruptFrameError*, const char* panicMessage);

#endif
