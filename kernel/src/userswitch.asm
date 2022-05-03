    [BITS 64]

;;; Very useful resource: https://nfil.dev/kernel/rust/coding/rust-kernel-to-userspace-and-back/
;;; (Check wayback machine if link is dead, I've archived it)

    extern tss                  ; Pointer to 64-bit TSS Entry structure.

jump_to_userland_function:
    cli
    
    mov rcx, rsp                ; Store kernel stack pointer to return to.
    
    mov rax, rcx
    mov rbx, [tss]              ; Store pointer to TSS Entry structure in `rbx`.
    mov DWORD [rbx + 4], eax    ; Store low 32 bits of stack pointer in `l_RSP0` field of TSS.
    shr rax, 32                 ; `eax` = high 32 bits of stack pointer.
    mov DWORD [rbx + 8], eax    ; Store high 32 bits of stack pointer in `h_RSP0` field of TSS.
    
    mov ax, 0x28                ; `ax` = GDT offset of TSS Entry.
    ltr ax                      ; Load GDT offset into task register (TSSR).

    xor rax, rax                ; Zero out entire 64-bit 'A' register.
    mov ax, 0x20 | 0b11         ; Store Ring 3 User Data GDT offset in segment registers.
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    mov rax, rcx
    push 0x20 | 0b11            ; GDT Offset of Ring 3 User Data entry.
    push rax
    pushfq                      ; Store the CPU Flags register on the stack.
    pop rax                     ; `rax` = CPU Flags register.
    or rax, 0b1000000000        ; Re-enable interrupts when after jump.
    push rax                    ; Set CPU Flags state to this after `iretq` jumps.
    push 0x18 | 0b11            ; GDT Offset of Ring 3 User Code entry.
    push rdi                    ; Address to return to
    iretq

GLOBAL jump_to_userland_function
