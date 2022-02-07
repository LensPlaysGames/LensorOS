    [BITS 64]
    
    extern syscalls             ; Table of system call functions defined in "syscalls.h"
    extern num_syscalls         ; Number of system call functions defined within syscalls table.

do_swapgs:
    cmp QWORD [rsp + 0x08], 0x08
    je skip_swap
    swapgs
skip_swap:
    ret

;;; System Call Handler
;;; Registers Used:
;;;   rax  --  System Call Code
;;; 
;;; System Call Code: Index offset into syscalls table.
system_call_handler_asm:        ; Accepts system call number in register 'A' (rax)
    cmp rax, [num_syscalls]       ; Number of system calls
    jae invalid_system_call
    ;; Save CPU state to be restored after system call.
    ;; TODO: `swapgs`
    ;; call do_swapgs
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
    ;; Actually execute the system call.
    call [syscalls + rax * 8]   ; 8 = sizeof(pointer) in 64 bit
    ;; Restore CPU state, as system call is no longer executing.
    add rsp, 8                  ; Eat `rsp` off the stack
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
    add rsp, 8                  ; Eat `rax` off the stack
    ;; TODO: `swapgs`
    ;; call do_swapgs
    iretq                       ; iretq -> interrupt return quad word (64 bit)

invalid_system_call:            ; If system call code is invalid, do nothing
    iretq

GLOBAL system_call_handler_asm
