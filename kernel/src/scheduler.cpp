#include <scheduler.h>

#include <debug.h>
#include <integers.h>
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
void(*scheduler_switch_process)(CPUState*)
    __attribute__((no_caller_saved_registers));

void(*timer_tick)();

namespace Scheduler {
    // Not the best, but wouldn't be a problem unless ridiculous uptime.
    pid_t the_pid { 0 };
    pid_t request_pid() {
        return the_pid++;
    }

    Process StartupProcess;

    SinglyLinkedList<Process*>* ProcessQueue { nullptr };
    SinglyLinkedListNode<Process*>* CurrentProcess { nullptr };

    void print_debug() {
        dbgmsg_s("[SCHED]: Debug information:\r\n"
                 "  Process Queue:\r\n");
        ProcessQueue->for_each([](auto* it) {
            Process& process = *it->value();
            dbgmsg("    Process %ull:\r\n"
                   "      CR3:      %x\r\n"
                   "      RAX:      %x\r\n"
                   "      RBX:      %x\r\n"
                   "      RCX:      %x\r\n"
                   "      RDX:      %x\r\n"
                   "      RSI:      %x\r\n"
                   "      RDI:      %x\r\n"
                   "      RBP:      %x\r\n"
                   "      RSP:      %x\r\n"
                   "      R8:       %x\r\n"
                   "      R9:       %x\r\n"
                   "      R10:      %x\r\n"
                   "      R11:      %x\r\n"
                   "      R12:      %x\r\n"
                   "      R13:      %x\r\n"
                   "      R14:      %x\r\n"
                   "      R15:      %x\r\n"
                   "      Frame:\r\n"
                   "        RIP:    %x\r\n"
                   "        CS:     %x\r\n"
                   "        RFLAGS: %x\r\n"
                   "        RSP:    %x\r\n"
                   "        SS:     %x\r\n"
                   , process.ProcessID
                   , process.CR3
                   , process.CPU.RAX
                   , process.CPU.RBX
                   , process.CPU.RCX
                   , process.CPU.RDX
                   , process.CPU.RSI
                   , process.CPU.RDI
                   , process.CPU.RBP
                   , process.CPU.RSP
                   , process.CPU.R8
                   , process.CPU.R9
                   , process.CPU.R10
                   , process.CPU.R11
                   , process.CPU.R12
                   , process.CPU.R13
                   , process.CPU.R14
                   , process.CPU.R15
                   , process.CPU.Frame.ip
                   , process.CPU.Frame.cs
                   , process.CPU.Frame.flags
                   , process.CPU.Frame.sp
                   , process.CPU.Frame.ss
                   );
        });
        dbgmsg_s("\r\n");
    }

    void add_process(Process* process) {
        process->ProcessID = request_pid();
        ProcessQueue->add(process);
        dbgmsg_s("[SCHED]: Added process.\r\n");
        print_debug();
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

        dbgmsg("Old Interrupt frame:\r\n"
               "  Instruction Pointer:   %x\r\n"
               "  Code Segment Selector: %x\r\n"
               "  Flags:                 %x\r\n"
               "  Stack Pointer:         %x\r\n"
               "  Data Segment Selector: %x\r\n"
               , cpu->Frame.ip
               , cpu->Frame.cs
               , cpu->Frame.flags
               , cpu->Frame.sp
               , cpu->Frame.ss
               );

        memcpy(cpu, &CurrentProcess->value()->CPU, sizeof(CPUState));
        if (CurrentProcess->next() == nullptr) {
            if(CurrentProcess == ProcessQueue->head())
                return;

            CurrentProcess = ProcessQueue->head();
        }
        else CurrentProcess = CurrentProcess->next();
        memcpy(&CurrentProcess->value()->CPU, cpu, sizeof(CPUState));
        Memory::flush_page_map(CurrentProcess->value()->CR3);

        asm("xor %%rax, %%rax\r\n\t"
            "movq %0, %%rax\r\n\t"
            "movw %%ax, %%es\r\n\t"
            "movw %%ax, %%ds\r\n\t"
            :: "r"(cpu->Frame.ss)
            : "rax"
            );

        cpu->FS = cpu->Frame.ss;
        cpu->GS = cpu->Frame.ss;

        dbgmsg("New Interrupt frame:\r\n"
               "  Instruction Pointer:   %x\r\n"
               "  Code Segment Selector: %x\r\n"
               "  Flags:                 %x\r\n"
               "  Stack Pointer:         %x\r\n"
               "  Data Segment Selector: %x\r\n"
               , cpu->Frame.ip
               , cpu->Frame.cs
               , cpu->Frame.flags
               , cpu->Frame.sp
               , cpu->Frame.ss
               );

        dbgmsg_s("Switched processes\r\n");
    }
}
