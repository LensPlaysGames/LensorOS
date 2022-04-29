#include <scheduler.h>

#include <debug.h>
#include <interrupts/idt.h>
#include <interrupts/interrupts.h>
#include <linked_list.h>
#include <memory.h>
#include <memory/paging.h>
#include <memory/virtual_memory_manager.h>
#include <pit.h>
#include <tss.h>
#include <uart.h>

/// External symbol definitions for `scheduler.asm`
void(*scheduler_switch_process)(CPUState*)__attribute__((interrupt));
void(*timer_tick)();

namespace Scheduler {
    Process StartupProcess;

    SinglyLinkedList<Process*>* ProcessQueue { nullptr };
    SinglyLinkedListNode<Process*>* CurrentProcess { nullptr };

    void add_process(Process* process) {
        ProcessQueue->add(process);
    }

    bool initialize() {
        // IRQ handler in assembly increments PIT ticks counter using this function.
        timer_tick = pit_tick;
        // IRQ handler in assembly switches processes using this function.
        scheduler_switch_process = scheduler_switch;
        // Setup currently executing code as the start process.
        StartupProcess.CR3 = Memory::active_page_map();
        // Create the process queue and add the startup process to it.
        ProcessQueue = new SinglyLinkedList<Process*>;
        if (ProcessQueue == nullptr) {
            dbgmsg_s("\033[31mScheduler failed to initialize:\033[0m Could not create process list.\r\n");
            return false;
        }

        add_process(&StartupProcess);
        CurrentProcess = ProcessQueue->head();
        // Install IRQ0 handler found in `scheduler.asm` (over-write default system timer handler).
        gIDT.install_handler((u64)irq0_handler, PIC_IRQ0);
        gIDT.flush();
        dbgmsg_s("Flushed IDT after installing new IRQ0 handler\r\n");
        return true;
    }

    /// Called from `irq0_handler` in `scheduler.asm`
    /// A stupid simple round-robin process switcher.
    void switch_process(CPUState* cpu) {
        dbgmsg_s("Switching processes\r\n");

        memcpy(&cpu, &CurrentProcess->value()->CPU, sizeof(CPUState));
        if (CurrentProcess->next() == nullptr) {
            if(CurrentProcess == ProcessQueue->head())
                return;

            CurrentProcess = ProcessQueue->head();
        }
        else CurrentProcess = CurrentProcess->next();
        memcpy(&CurrentProcess->value()->CPU, &cpu, sizeof(CPUState));
        Memory::flush_page_map(CurrentProcess->value()->CR3);

        dbgmsg_s("Switched processes\r\n");
    }
}

// This work-around/hack is due to needing a function pointer
void scheduler_switch(CPUState* cpu) {
    Scheduler::switch_process(cpu);
};
