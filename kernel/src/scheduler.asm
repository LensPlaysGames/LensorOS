    [BITS 64]

    ;; External symbols provided in `scheduler.h` && `scheduler.cpp`
    ;; A pointer to task switching handler function.
    extern scheduler_switch_func
    ;; Total amount of times IRQ0 has been called since boot.
    extern timer_ticks

do_swapgs:
    cmp QWORD [rsp + 0x08], 0x08
    je skip_swap
    swapgs
skip_swap:
    ret

irq0_handler:
;;; SAVE CPU STATE ON STACK
    ;; Already on the stack thanks to interrupt:
    ;; |- Old GDT Segment Selector
    ;; |- Old Stack Pointer (RSP)
    ;; |- Flags Register (RFLAGS)
    ;; |- Code Segment Selector
    ;; |- Instruction Pointer (RIP)
    ;; `- Error Code
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
    mov rax, [timer_ticks]
    add QWORD [rax], 1
    mov [timer_ticks], rax
;;; CALL C++ FUNCTION; ARGUMENT IN `rdi`
    mov rdi, rsp
    call [scheduler_switch_func]
;;; END INTERRUPT
    mov ax, 0x20
    out 0x20, ax
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
    iretq

GLOBAL irq0_handler
