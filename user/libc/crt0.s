    .section .text
    .global _start
_start:
    ;# Stack frame linked list null entry.
    movq $0, %rbp
    pushq %rbp
    pushq %rbp
    movq %rsp, %rbp

    ;# Store argc/argv
    pushq %rsi
    pushq %rdi

    ;# call initialize_standard_library

    ;# Run global constructors.
    call _init

    popq %rdi
    popq %rsi

    call main

    movl %eax, %edi
    call exit
.size _start, . - _start
