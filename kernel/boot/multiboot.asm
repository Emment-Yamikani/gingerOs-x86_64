[bits 32]

MODS_ALIGN  EQU (1 << 0)
MEMINFO     EQU (1 << 1)
GFXMODE     EQU (1 << 2)
MAGIC       EQU (0x1BADB002);(0xE85250D6);
FLAGS       EQU (MODS_ALIGN | MEMINFO | GFXMODE)
CHECKSUM    EQU -(MAGIC + FLAGS)

section .multiboot
align 4

mboot:
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

; section header info(Not of concern)

    dd 0x00
    dd 0x00
    dd 0x00
    dd 0x00
    dd 0x00

; gfx parameters
    LFB_GFX     EQU (0)
    EGA_TXT     EQU (1)
    GFX_WIDTH   EQU (1024)
    GFX_HEIGHT  EQU (800)
    GFX_DEPTH   EQU (32)

    dd EGA_TXT
    dd GFX_WIDTH
    dd GFX_HEIGHT
    dd GFX_DEPTH
;

PGSZ    EQU 0X1000
VMABASE EQU 0xC0000000

extern x86_64_init

global  _start
section ._linit
_start:
    cli
    lea ebp, [bootstrap_stack - VMABASE]
    mov esp, ebp

    push eax
    push ebx

    lea eax, [x86_64_init - VMABASE]
    call eax

    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    lea eax, [.higher_half32]
    jmp eax
.higher_half32:
    add esp, VMABASE
    add ebp, VMABASE

    pop ebx
    pop eax

    lgdt [gdt64.pointer - VMABASE]
    lea eax, start_64bit
    jmp gdt64.code : start_64bit

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
        dq (gdt64 - VMABASE)

[bits 64]
start_64bit:
    cli
    mov ax, gdt64.data
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rax, kernel_start
    jmp rax

align PGSZ
times PGSZ db 0
bootstrap_stack:

extern cga_init
extern early_init
section .text
[bits 64]
kernel_start:
    cli
    mov rsp, cpu0_stack.top
    mov rbp, rsp
    
    push rbx
    call cga_init
    pop rbx
    
    mov rax, qword 0xffff800000000000
    or rax, rbx
    xchg rax, rbx
    mov rdi, rbx
    call early_init
    hlt
    jmp $

section .bss
align PGSZ
cpu0_stack:
resb    0x80000 ;512KiB of stack
.top: