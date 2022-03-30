#include "scheduler.h"

#include "cstr.h"
#include "interrupts/idt.h"
#include "linked_list.h"
#include "memory.h"
#include "memory/paging.h"
#include "memory/virtual_memory_manager.h"
#include "pit.h"
#include "uart.h"

/// External symbol definitions for `scheduler.asm`
void* scheduler_switch_process;
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
        timer_tick = &pit_tick;
        // IRQ handler in assembly switches processes using this function.
        scheduler_switch_process = (void*)switch_process;
        // Setup currently executing code as the start process.
        StartupProcess.CR3 = Memory::get_active_page_map();
        // Create the process queue and add the startup process to it.
        ProcessQueue = new SinglyLinkedList<Process*>;
        add_process(&StartupProcess);
        CurrentProcess = ProcessQueue->head();
        // Confidence checks.
        if (ProcessQueue == nullptr || ProcessQueue->head() == nullptr)
            return false;

        // Install IRQ0 handler found in `scheduler.asm` (over-write default system timer handler).
        gIDT.install_handler((u64)irq0_handler, PIC_IRQ0);
        return true;
    }

    /// Called from `irq0_handler` in `scheduler.asm`
    /// A stupid simple round-robin process switcher.
    void switch_process(CPUState* cpu) {
        memcpy(&cpu, &CurrentProcess->value()->CPU, sizeof(CPUState));
        if (CurrentProcess->next() == nullptr) {
            if(CurrentProcess == ProcessQueue->head())
                return;

            CurrentProcess = ProcessQueue->head();
        }
        else CurrentProcess = CurrentProcess->next();
        memcpy(&CurrentProcess->value()->CPU, &cpu, sizeof(CPUState));
        Memory::flush_page_map(CurrentProcess->value()->CR3);
    }
}
