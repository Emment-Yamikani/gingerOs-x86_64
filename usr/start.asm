bits 64

extern main
global start
extern sys_exit

section .text
start:
    call main
    mov rdi, rax
    call sys_exit
    jmp $

section .data
.__data_section: db 0