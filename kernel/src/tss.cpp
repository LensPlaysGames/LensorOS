#include "tss.h"

#include "memory.h"
#include "gdt.h"

TSSEntry tssEntry;
// USED IN `userswitch.asm` `jump_to_userland_function` AS EXTERNAL SYMBOL.
void* tss;

void TSS::initialize() {
    memset(&tssEntry, 0, sizeof(TSSEntry));
    u32 limit = sizeof(TSSEntry) - 1;
    u8 flags = gGDT.TSS.Entry.Limit1_Flags;
    u64 base = (u64)&tssEntry;
    gGDT.TSS.Entry.Limit0 = limit;
    gGDT.TSS.Entry.Limit1_Flags = limit >> 16;
    gGDT.TSS.Entry.Limit1_Flags |= flags;
    gGDT.TSS.Entry.Base0 = base;
    gGDT.TSS.Entry.Base1 = base >> 16;
    gGDT.TSS.Entry.Base2 = base >> 24;
    gGDT.TSS.Base3 = base >> 32;
    tss = &tssEntry;
}
