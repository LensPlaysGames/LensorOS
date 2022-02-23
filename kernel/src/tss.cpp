#include "tss.h"

#include "memory.h"
#include "gdt.h"

TSSEntry tssEntry;
// USED IN `userswitch.asm` `jump_to_userland_function` AS EXTERNAL SYMBOL.
void* tss;

void TSS::initialize() {
    tss = &tssEntry;
    // Zero out TSS entry.
    memset(&tssEntry, 0, sizeof(TSSEntry));
    // Set byte limit of TSS Entry past base address.
    gGDT.TSS.set_limit(sizeof(TSSEntry) - 1);
    // Set base address to address of TSS Entry.
    u64 base = (u64)&tssEntry;
    gGDT.TSS.set_base(base);
    gGDT.TSS.Base3 = base >> 32;
}
