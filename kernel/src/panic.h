#ifndef LENSOR_OS_PANIC_H
#define LENSOR_OS_PANIC_H

#define PanicStartX 400
#define PanicStartY 500

class InterruptFrame;
class InterruptFrameError;

void panic(const char* panicMessage);
void panic(InterruptFrame*, const char* panicMessage);
void panic(InterruptFrameError*, const char* panicMessage);

#endif
