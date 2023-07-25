[bits 16]

section .trampoline

SEG_ACC     EQU (1 << 0)
SEG_RW      EQU (1 << 1)
SEG_DC      EQU (1 << 2)
SEG_CODE    EQU (1 << 3)

SEG_TYPE    EQU (1 << 4)
SEG_DPL0    EQU (0 << 5)
SEG_PRES    EQU (1 << 7)

SEG_LONG    EQU (1 << 5)
SEG_DB      EQU (1 << 6)
SEG_GRAN    EQU (1 << 7)

align 4
global ap_trampoline
ap_trampoline:
    cli
    cld

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    mov eax, dword [pdbr]
    mov cr3, eax

    lgdt [gdt64.pointer]

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    jmp gdt64.code:long_mode

gdt64:
    .null: equ $ - gdt64
        dq 0
    .code: equ $ - gdt64
        dw 0xFFFF ; limit_lo
        dw 0      ; base_lo
        db 0      ; base_mid
        db (SEG_PRES | SEG_DPL0 | SEG_TYPE | SEG_CODE | SEG_RW)
        db (SEG_GRAN | SEG_LONG | 0XF)
        db 0
    .data: equ $ - gdt64
        dw 0xFFFF
        dw 0
        db 0
        db (SEG_PRES | SEG_DPL0 | SEG_TYPE | SEG_RW)
        db (SEG_GRAN | SEG_DB | 0XF)
        db 0
    .pointer:
        dw $ - gdt64 - 1
        dq (gdt64)

[bits 64]

long_mode:
    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov fs, ax 
    mov gs, ax
    mov ss, ax

    mov rsp, qword [stack]
    mov rbp, rsp

    retq

times 4032 - ($ - $$) db 0
pdbr:  dq 0 ; pdbr
stack: dq 0 ; AP bootstrap stack.