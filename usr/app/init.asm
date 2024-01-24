bits 64

global main
extern sys_putc

section .text
main:
    mov rdi, 'u'
    call sys_putc
    jmp $