[bits 64]

global rdcr0
global wrcr0

global rdcr2
global wrcr2

global rdcr3
global wrcr3

global rdcr4
global wrcr4


global disable_caching

global rdrax
rdrax:
    ret
global rdrflags
rdrflags:
    pushfq
    pop qword rax
    ret

global wrrflags
wrrflags:
    push qword rdi
    popfq
    ret

rdcr0:
    mov rax, cr0
    ret

wrcr0:
    mov cr0, rdi
    ret

rdcr2:
    mov rax, cr2
    ret

wrcr2:
    mov cr2, rdi
    ret

rdcr3:
    mov rax, cr3
    ret

wrcr3:
    mov cr3, rdi
    ret

rdcr4:
    mov rax, cr4
    ret

wrcr4:
    mov cr4, rdi
    ret

global cpuid
cpuid:
    push rbx    ;&
    push rcx    ;&ebx
    push rdx    ;&eax

    mov eax, edi
    mov ecx, esi

    cpuid

    pop rdi
    mov dword [rdi], eax
    pop rdi
    mov dword [rdi], ebx
    pop rbx
    mov dword [r8], ecx
    mov dword [r9], edx
ret

disable_caching:
    mov rax, cr0
    and eax, 0x8000000f
    mov cr0, rax
    ret

global wrmsr
global rdmsr

wrmsr:
    mov rcx, rdi
    mov rax, rsi
    mov rdx, rax
    shr rdx, 32
    wrmsr
    ret

rdmsr:
    mov rcx, rdi
    rdmsr
    shl rdx, 32
    or rdx, rax
    xchg rdx, rax
    ret

global loadgdt64 ; loadgdt64(gdtptr, cs, gs, ss)
loadgdt64:
    lgdt [rdi]
    push qword rcx
    push qword rsp
    pushfq
    push qword rsi
    mov rax, qword .switch
    push qword rax
    iretq
    .switch:
        mov rax, rcx
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov ss, ax
        mov rax, rdx
        mov gs, ax
        add rsp, 8
    ret

global loadidt
loadidt:
    lidt [rdi]
    ret

global loadtr
loadtr:
    ltr di
    ret

global invlpg
invlpg:
invlpg [rdi]
ret