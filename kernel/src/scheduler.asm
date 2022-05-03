    [BITS 64]
    ;; External symbols provided in `scheduler.h` and `scheduler.cpp`
    ;; A pointer to task switching handler function.
    extern scheduler_switch_process
    ;; A pointer to a function that increments timer ticks by one.
    extern timer_tick
do_swapgs:
    cmp QWORD [rsp + 0x8], 0x8
    je skip_swap
    swapgs
skip_swap:
    ret

    GLOBAL irq0_handler
irq0_handler:
    ;; `iretq` arguments already on the stack:
    ;; |-- Data Segment Selector
    ;; |-- Old Stack Pointer (RSP)
    ;; |-- Flags Register (RFLAGS)
    ;; |-- Code Segment Selector
    ;; `-- Instruction Pointer (RIP)
;;; SAVE CPU STATE ON STACK
    call do_swapgs
    push rax
    push gs
    push fs
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rsp
;;; INCREMENT SYSTEM TIMER TICKS
    call [rel timer_tick]
;;; CALL C++ FUNCTION; ARGUMENT IN `rdi`
    mov rdi, rsp
    call [rel scheduler_switch_process]
;;; END INTERRUPT
    mov ax, 0x20                ; 0x20 = PIC_EOI
    out 0x20, al                ; 0x20 = PIC1_COMMAND port
;;; RESTORE CPU STATE FROM STACK
    add rsp, 8                  ; Eat `rsp` off of stack.
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop fs
    pop gs
    pop rax
    call do_swapgs
    ;; #GP(selector) with error code of `0x18` at `iretq`
    ;; From the manual:
    ;; #GP(selector) If a segment selector index is outside its descriptor table limits.
    ;;                   I dont think it's this due to the "limits" being set to the highest possible number.
    ;;
    ;;               If the return code segment selector RPL is less than the CPL.
    ;;                   RPL = request privilege level, and is set by the last 2 bits of the segment selector.
    ;;                   I don't think it's this due to the CPL being zero, the minimum.
    ;;
    ;;               If the DPL of a conforming-code segment is greater than the return code segment selector RPL.
    ;;                   DPL = descriptor privilege level, and is set in the access byte of the GDT descriptor.
    ;;                   I don't think it's this due to every GDT entry being non-conforming.
    ;;
    ;;               If the DPL for a nonconforming-code segment is not equal to the RPL of the code segment selector.
    ;;                   DPL = 0b11, RPL = 0b00
    ;;                   THIS COULD BE A PROBLEM
    ;;
    ;;               If the stack segment descriptor DPL is not equal to the RPL of the return code segment selector.
    ;;                   Code/Data segments must have matching privilege level.
    ;;                   I don't think it's this due to the ring 3 code/data segments having the same DPL set (0b11).
    ;;
    ;;               If the stack segment is not a writable data segment.
    ;;                   I don't think it's this due to the GDT data segments having the read/write bit set.
    ;;
    ;;               If the stack segment selector RPL is not equal to the RPL of the return code segment selector.
    ;;                   Requested privilege level of SS must match RPL of code segment.
    ;;                   THIS COULD BE A PROBLEM
    ;;
    ;;               If the segment descriptor for a code segment does not indicate it is a code segment.
    ;;                   I don't think it's this due to the GDT code segments having the executable/code bit set.
    ;;
    ;;               If the segment selector for a TSS has its local/global bit set for local.
    ;;                   I don't actually know what the local/global bit is???
    ;;                   THIS COULD BE A PROBLEM
    ;;
    ;;               If a TSS segment descriptor specifies that the TSS is not busy.
    ;;                   What causes the TSS to be busy? I assume some sort of bit flag, but which one?
    ;;                   THIS COULD BE A PROBLEM
    ;;
    ;;               If a TSS segment descriptor specifies that the TSS is not available.
    ;;                   What does not available mean? Is it the available bit flag?
    ;;                   THIS COULD BE A PROBLEM
    iretq
