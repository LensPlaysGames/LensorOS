    [BITS 64]

    extern userland_function    ; Pointer to a function that will execute in user mode.
    extern tss                  ; Pointer to 64-bit TSS Entry structure.

    ;; TODO: Page Table Swap (or at least TLB)
jump_to_userland_function:
    cli

    mov rcx, rsp                ; Store kernel stack pointer to return to.
    
    mov rax, rcx
    mov rbx, [tss]              ; Store pointer to TSS Entry structure in `rbx`.
    mov DWORD [rbx + 4], eax    ; Store low 32 bits of stack pointer in `l_RSP0` field of TSS.
    shr rax, 32                 ; `eax` = high 32 bits of stack pointer.
    mov DWORD [rbx + 8], eax    ; Store high 32 bits of stack pointer in `h_RSP0` field of TSS.
    
    mov ax, 0x30
    ltr ax

    xor rax, rax
    mov ax, 0x28 | 0b11
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov rax, rcx
    push 0x28 | 0b11
    push rax
    pushfq
    pop rax
    or rax, 0b1000000000
    push rax
    push 0x20 | 0b11
    push QWORD [userland_function]
    iretq

GLOBAL jump_to_userland_function
