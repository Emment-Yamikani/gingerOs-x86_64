[bits 32]

global __pml4
global start32
extern cga_init
extern early_init

PGSZ EQU 0x1000
VMA  EQU 0xFFFF800000000000

align 4
section .text

start32:
    cli
    cld

    mov esp, (stack.top - VMA)
    mov ebp, esp
    push dword 0
    push dword eax
    push dword 0
    push dword ebx

    ; Just make sure paging structures are zero'ed,
    ; Even though the bootloader does that for us,
    ; You can never be too sure (I have servere trust issues ;) ).
    xor eax, eax
    mov edi, (__pml4 - VMA)
    mov cr3, edi
    mov ecx, 0xC00
    rep stosd

    mov edi, (__pml4 - VMA)
    mov eax, edi
    add eax, PGSZ

    or eax, 3

    mov dword [edi], eax
    mov dword [edi + 0x800], eax

    add edi, PGSZ
    add eax, PGSZ

    mov dword [edi], eax
    
    add edi, PGSZ
    xor eax, eax
    or eax, 0x83    ; use 2mb pages
    mov ecx, 512
.map:
    mov dword[edi], eax
    add edi, 8
    add eax, 0x200000
    loop .map

    mov eax, 0x80000000
    cpuid

    test eax, 0x80000001 ; Test for extended cpu features.
    jb .noext

    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)
    jz .no64

    mov eax, cr4
    or eax, (3 << 4) ; CR4.PAE | CR4.PSE.
    mov cr4, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)    ; EFER.LM = 1.
    wrmsr

    mov eax, cr0
    and eax, 0x0fffffff ; disable per-cpu caching
    or eax, 0x80000000  ; enable PML4 paging.
    mov cr0, eax

    mov eax, (gdtbase - VMA)
    lgdt [eax]
    jmp 0x8:(start64 - VMA)

.noext:
    mov eax, 0xDEADCAFE
    jmp $
.no64:
    mov eax, 0xDEADBEEF
    hlt
    jmp $

gdt64:
    .null dq 0
    .code dq 0xAF9A000000FFFF
    .data dq 0xCF92000000FFFF
gdtbase:
        dw (gdtbase - gdt64) - 1
        dq gdt64

[bits 64]

align 16
start64:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    mov rax, .high64
    jmp rax

.high64:
    pop rdi
    pop rax
    mov rsp, stack.top
    mov rbp, rsp
    
    push rdi
    push rax
    
    call cga_init
    
    pop rax
    mov rcx, rax
    pop rdi
    
    call early_init

    jmp $

align 16
section .bss
stack:
    resb 0x80000 ; 512 Kib kernel stack per-thread
.top:

align 0x1000
__pml4:
resb PGSZ * 3