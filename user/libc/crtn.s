.section .init
	;# GCC will put .init section of `crtend.o` here.
	popq %rbp
	ret

.section .fini
	;# GCC will put .fini section of `crtend.o` here.
	popq %rbp
	ret	
