[bits 32]

PGSZ    equ 0x1000
VMABASE equ 0xC0000000

section ._linit

global x86_64_init

x86_64_init:
    cli
    lea eax, [x86_64_pml4_init - VMABASE]
    call eax
    ret

global x86_64_pml4_init
x86_64_pml4_init:
    pusha

    pushfd
    pop eax

    mov ecx, eax

    xor eax, (1 << 21)

    push eax
    popfd

    pushfd
    pop eax

    push ecx
    popfd

    xor eax, ecx
    jz .nocpuid

    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .nolongmode

    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)
    jz .nolongmode

    mov eax, cr0
    and eax, 0x3FFFFFFF
    mov cr0, eax
    lea edi, [__pml4 - VMABASE]
    mov cr3, edi
    mov ecx, 0xC00
    xor eax, eax
    rep stosd

    lea edi, [__pml4 - VMABASE]           ; edi points to __pml4
    lea eax, [(__pml4 - VMABASE) + PGSZ]  ; eax points to pdpt
    or eax, 3

    mov dword [edi], eax
    mov dword [edi + 0x800], eax

    add edi, PGSZ   ; edi now points to pdpt
    add eax, PGSZ   ; eax now points to pdt

    mov dword [edi], eax
    mov dword [edi + 0x18], eax

    add edi, PGSZ   ; edi now points to pdt
    xor eax, eax    ; eax points to 0
    or eax, 0x83
    mov ecx, 512
    .map0:
        mov dword[edi], eax
        add eax, 0x200000
        add edi, 8
        loop .map0

    ; Enable PAE
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    popa
    ret

.nocpuid:
    popa
    mov eax, -1
    ret

.nolongmode:
    popa
    mov eax, -2
    ret

align PGSZ
global __pml4
__pml4:
times (PGSZ * 3) - ($ - __pml4) db 0