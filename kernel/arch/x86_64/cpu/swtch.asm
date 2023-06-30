global swtch
swtch:
    push rbp
    push rbx
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov qword[rdi], rsp
    mov rsp, rsi

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop rbx
    pop rbp

    ret