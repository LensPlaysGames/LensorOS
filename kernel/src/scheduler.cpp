#include "scheduler.h"

#include "cstr.h"
#include "memory.h"
#include "pit.h"
#include "uart.h"

// External symbols for `scheduler.asm`
void* scheduler_switch_func;
u64* timer_ticks;

KernelProcess StartupProcess;

KernelProcess* Scheduler::ProcessQueue   { nullptr };
KernelProcess* Scheduler::CurrentProcess { nullptr };

void Scheduler::add_kernel_process(KernelProcess* process) {
    KernelProcess* last = ProcessQueue;
    while (CurrentProcess->Next != nullptr) {
        last = CurrentProcess->Next;
    }
    last->Next = process;
}

/// Called from System Timer Interrupt (IRQ0)
void Scheduler::switch_process(CPUState* cpu) {
    memcpy(&cpu, &CurrentProcess->CPU, sizeof(CPUState));
    if (CurrentProcess->Next == nullptr)
        CurrentProcess = ProcessQueue;
    else CurrentProcess = CurrentProcess->Next;
    memcpy(&CurrentProcess->CPU, &cpu, sizeof(CPUState));
    asm volatile ("mov %0, %%cr3" : : "r" (CurrentProcess->CR3));
}

void Scheduler::initialize(PageTable* bootPageMap) {
    ProcessQueue = &StartupProcess;
    CurrentProcess = &StartupProcess;
    // IRQ handler in assembly needs to increment PIT ticks counter.
    timer_ticks = &gPIT.Ticks;
    // IRQ handler in assembly needs to call a function to switch process.
    scheduler_switch_func = (void*)switch_process;
    // Setup start process as current executing code.
    StartupProcess.CR3 = bootPageMap;
    StartupProcess.Next = nullptr;
}
