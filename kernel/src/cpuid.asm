[BITS 64]    
SECTION .text
GLOBAL cpuid_support
cpuid_support:
    pushfq                      ; Save RFLAGS

    pushfq                      ; Store RFLAGS on stack
    xor QWORD [rsp], 0x00200000 ; Invert stored EFLAGS 'ID' bit
    popfq                       ; Try to load stored RFLAGS with modified 'ID' bit

    pushfq                      ; Store RFLAGS on stack to check if 'ID' has been changed
    pop rax                     ; Store RFLAGS from stack into RAX
    
    xor rax, [rsp]              ; RAX now equals altered bits of stored RFLAGS vs saved RFLAGS
    
    popfq                       ; Restore original RFLAGS   

    and rax, 0x200000           ; RAX now equals zero if 'ID' bit can't be changed (CPUID not supported)     
    ret
