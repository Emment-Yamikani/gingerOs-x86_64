bits 64

global main
extern printf

section .text
main:
    lea rdi, qword [fmt]
    call printf
    jmp $

fmt db "Hello init", 0xa, 0