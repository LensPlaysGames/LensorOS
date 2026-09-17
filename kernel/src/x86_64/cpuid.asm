;; Copyright 2022, Contributors To LensorOS.
;; All rights reserved.
;;
;; This file is part of LensorOS.
;;
;; LensorOS is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; LensorOS is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with LensorOS. If not, see <https://www.gnu.org/licenses

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
