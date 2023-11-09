[bits 64]
section .text

%macro ISR_ERR 1
global isr%1
isr%1:
    push    qword %1
    jmp     stub
%endmacro

%macro ISR_NOERR 1
global isr%1
isr%1:
    push    qword 0
    push    qword %1
    jmp     stub
%endmacro

%macro IRQ 2
global irq%1
irq%1:
    push    qword 0
    push    qword %2
    jmp     stub
%endmacro

ISR_NOERR 0
    ISR_NOERR 1
    ISR_NOERR 2
    ISR_NOERR 3
    ISR_NOERR 4
    ISR_NOERR 5
    ISR_NOERR 6
    ISR_NOERR 7
    ISR_ERR   8
    ISR_NOERR 9
    ISR_ERR   10
    ISR_ERR   11
    ISR_ERR   12
    ISR_ERR   13
    ISR_ERR   14
    ISR_NOERR 15
    ISR_NOERR 16
    ISR_NOERR 17
    ISR_NOERR 18
    ISR_NOERR 19
    ISR_NOERR 20
    ISR_NOERR 21
    ISR_NOERR 22
    ISR_NOERR 23
    ISR_NOERR 24
    ISR_NOERR 25
    ISR_NOERR 26
    ISR_NOERR 27
    ISR_NOERR 28
    ISR_NOERR 29
    ISR_NOERR 30
    ISR_NOERR 31
ISR_NOERR 128

IRQ 0, 32
    IRQ 1, 33
    IRQ 2, 34
    IRQ 3, 35
    IRQ 4, 36
    IRQ 5, 37
    IRQ 6, 38
    IRQ 7, 39
    IRQ 8, 40
    IRQ 9, 41
    IRQ   10, 42
    IRQ   11, 43
    IRQ   12, 44
    IRQ   13, 45
    IRQ   14, 46
    IRQ 15, 47
    IRQ 16, 48
    IRQ 17, 49
    IRQ 18, 50
    IRQ 19, 51
    IRQ 20, 52
    IRQ 21, 53
    IRQ 22, 54
    IRQ 23, 55
    IRQ 24, 56
    IRQ 25, 57
    IRQ 26, 58
    IRQ 27, 59
    IRQ 28, 60
    IRQ 29, 61
    IRQ 30, 62
IRQ 31, 63

global trapret
extern trap

stub:
    swapgs
    push    fs

    push    qword rax
    push    qword rbx
    push    qword rcx
    push    qword rdx
    push    qword rdi
    push    qword rsi
    push    qword rbp
    push    qword r8
    push    qword r9
    push    qword r10
    push    qword r11
    push    qword r12
    push    qword r13
    push    qword r14
    push    qword r15

    mov     rdi, rsp
    call    trap

trapret:
    pop     qword r15
    pop     qword r14
    pop     qword r13
    pop     qword r12
    pop     qword r11
    pop     qword r10
    pop     qword r9
    pop     qword r8
    pop     qword rbp
    pop     qword rsi
    pop     qword rdi
    pop     qword rdx
    pop     qword rcx
    pop     qword rbx
    pop     qword rax

    pop     fs
    swapgs
    add     rsp, 16
    iretq