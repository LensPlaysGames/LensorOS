.section .init
.global _init
_init:
	pushq %rbp
	movq %rsp, %rbp
	;# GCC will put .init section of `crtbegin.o` here.


.section .fini
.global _fini
_fini:
	pushq %rbp
	movq %rsp, %rbp
	;# GCC will put .fini section of `crtbegin.o` here.
