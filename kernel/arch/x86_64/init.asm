section .text

global _PML4_
global start32

extern cga_init
extern multiboot_info_process
extern earlycons_init
extern early_init
extern x86_64_init

PGSZ    equ 0x1000
DEVADDR equ 0xFE000000
VMA     equ 0xFFFFFF8000000000

[bits 32]
start32:
    cli
    cld

    mov     esp, (stack.top - VMA)
    mov     ebp, esp
    push    dword 0x0
    push    eax
    push    dword 0xFFFFFF80
    push    ebx

    xor     eax, eax
    mov     edi, (_PML4_ - VMA)
    mov     cr3, edi
    mov     ecx, 0x101000
    rep     stosd

    mov     edi, (_PML4_ - VMA)
    mov     eax, edi
    or      eax, 0x3
    mov     dword [edi + 0xFF0], eax    ; PML4E510 -> _PML4_

    mov     eax, (PDPT - VMA)
    or      eax, 0x3
    mov     dword [edi], eax            ; PML4E0 -> PDPT0
    mov     dword [edi + 0xFF8], eax    ; PML4E511 -> PDPT0
    
    mov     edi, (PDPT - VMA)
    mov     eax, (PDT - VMA)
    or      eax, 0x3

    mov     dword [edi], eax
    add     eax, PGSZ
    mov     dword [edi + 0x8], eax

    mov     edi, (PDT - VMA)
    mov     eax, (PT - VMA)
    or      eax, 0x3
    mov     ecx, 1024
    .mapt:
        mov     dword [edi], eax
        add     edi, 0x8
        add     eax, PGSZ
        loop    .mapt

    mov     edi, (PT - VMA)
    mov     eax, (0x0 | 0x3)
    mov     ecx, 0x80000 ; 2GiB worth of pages
    .map:
        mov     dword [edi], eax
        add     edi, 8
        add     eax, PGSZ
        loop    .map
    
    mov edi, (PDPT - VMA)
    mov eax, (DEVPDT - VMA)
    or eax, 0x1b
    mov dword[edi + 0x18], eax

    mov edi, (DEVPDT - VMA)
    mov eax, (DEVPT - VMA)
    or eax, 0x1b
    mov ecx, 16
    .mapdevpt:
        mov dword[edi + 0xF80], eax
        add edi, 8
        add eax, PGSZ
        loop .mapdevpt
    
    mov edi, (DEVPT - VMA)
    mov eax, (DEVADDR | 0x1b)
    mov ecx, 8192 ; dev pages.
    .mapdevs:
        mov dword[edi], eax
        add edi, 8
        add eax, PGSZ
        loop .mapdevs

    mov     eax, 0x80000000
    cpuid
    test    eax, 0x80000001 ; Test for extended cpu features.
    jb      .noext

    mov     eax, 0x80000001
    cpuid
    test    edx, (1 << 29)
    jz      .no64

    mov     eax, cr4
    or      eax, (3 << 4) ; CR4.PAE | CR4.PSE.
    mov     cr4, eax

    mov     ecx, 0xC0000080
    rdmsr
    or      eax, (1 << 8) ; EFER.LM = 1.
    wrmsr

    mov     eax, cr0
    and     eax, 0x0fffffff ; disable per-cpu caching
    or      eax, 0x80000000 ; enable PML4 paging.
    mov     cr0, eax

    mov     eax, (gdtbase - VMA)
    lgdt    [eax]
    jmp     0x8:(start64 - VMA)

.noext:
    mov     eax, 0xDEADCAFE
    jmp     $
.no64:
    mov     eax, 0xDEADBEEF
    hlt
    jmp     $

gdt64:
    .null   dq 0
    .code   dq 0xAF9A000000FFFF
    .data   dq 0xCF92000000FFFF
gdtbase:
        dw  (gdtbase - gdt64) - 1
        dq  gdt64

[bits 64]

align 16
start64:
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    
    mov     rax, .high64
    jmp     rax

.high64:
    mov     rdi, _PML4_
    mov     qword [rdi], 0 ; Unmap PML4E0
    invlpg  [0]

    mov     rsp, (stack.top - 0x10)
    mov     rbp, stack.top
    pop     rdi
    pop     rcx

    call    multiboot_info_process
    call    earlycons_init
    call    early_init
    
    cli
    hlt
    jmp $

align 16
section .bss
align PGSZ
_PML4_:
    resb PGSZ
PDPT:
    resb PGSZ
PDT:
    resb PGSZ * 2
PT:
    resb PGSZ * 1024
DEVPDT:
    resb PGSZ
DEVPT:
    resb PGSZ * 16

stack:
    resb 0x80000
    .top: