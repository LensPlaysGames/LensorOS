#include <tss.h>

#include <debug.h>
#include <link_definitions.h>
#include <memory.h>
#include <gdt.h>

TSSEntry tssEntry;
// USED IN `userswitch.asm` `jump_to_userland_function` AS EXTERNAL SYMBOL.
void* tss;

void TSS::initialize() {
    tss = &tssEntry;
    // Zero out TSS entry.
    memset(&tssEntry, 0, sizeof(TSSEntry));
    // Set byte limit of TSS Entry past base address.
    u64 limit = sizeof(TSSEntry) - 1;
    gGDT.TSS.set_limit(limit);
    // Set base address to address of TSS Entry.
    u64 base = V2P((u64)&tssEntry);
    //u64 base = (u64)&tssEntry;
    gGDT.TSS.set_base(base);
    dbgmsg("[TSS]: Initialized\r\n"
           "  Base:  %x\r\n"
           "  Limit: %x\r\n"
           , gGDT.TSS.base()
           , gGDT.TSS.limit()
           );
    // Store current stack pointer in TSS entry.
    u64 stackPointer { 0 };
    asm("movq %%rsp, %0\r\n\t"
        : "=m"(stackPointer)
        );
    tssEntry.l_RSP0 = stackPointer;
    tssEntry.h_RSP0 = stackPointer >> 32;
    asm("mov $0x28, %%ax\r\n\t"
        "ltr %%ax\r\n\t"
        ::: "rax"
        );
}
