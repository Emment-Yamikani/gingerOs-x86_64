global swtch
swtch:
    push    rbp
    push    rbx
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15
    
    sub     rsp, 8 ; For link.    

    mov     qword[rdi], rsp
    mov     rsp, rsi

    add     rsp, 8 ; For link.
    
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     rbx
    pop     rbp

    retq