#include "scheduler.h"

#include "cstr.h"
#include "memory.h"
#include "pit.h"
#include "uart.h"

// External symbols for `scheduler.asm`
void* scheduler_switch_task;
u64* timer_ticks;

KernelProcess StartupProcess;

KernelProcess* Scheduler::ProcessQueue   { &StartupProcess };
KernelProcess* Scheduler::CurrentProcess { &StartupProcess };

void Scheduler::add_task(KernelProcess* process) {
    KernelProcess* last = ProcessQueue;
    while (CurrentProcess->Next != nullptr) {
        last = CurrentProcess->Next;
    }
    last->Next = process;
}

/// Called from System Timer Interrupt (IRQ0)
void Scheduler::switch_task(CPUState* cpu) {
    memcpy(&cpu, &CurrentProcess->CPU, sizeof(CPUState));
    if (CurrentProcess->Next == nullptr)
        CurrentProcess = ProcessQueue;
    else CurrentProcess = CurrentProcess->Next;
    memcpy(&CurrentProcess->CPU, &cpu, sizeof(CPUState));
    asm volatile ("mov %0, %%cr3" : : "r" (CurrentProcess->CR3));
}

void Scheduler::Initialize(PageTable* bootPageMap) {
    // Setup external symbols.
    timer_ticks = &gPIT.Ticks;
    scheduler_switch_task = (void*)&switch_task;
    // Setup start process as current executing code.
    StartupProcess.CR3 = bootPageMap;
    StartupProcess.Next = nullptr;
}
