bits 64

extern main
global start

section .text
start:
    call main
    jmp $