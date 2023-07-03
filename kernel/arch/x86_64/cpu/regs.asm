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
;rdrax:
    retq
global rdrflags
rdrflags:
    pushfq
    pop qword rax
    retq

global wrrflags
wrrflags:
    push qword rdi
    popfq
    retq

rdcr0:
    mov rax, cr0
    retq

wrcr0:
    mov cr0, rdi
    retq

rdcr2:
    mov rax, cr2
    retq

wrcr2:
    mov cr2, rdi
    retq

rdcr3:
    mov rax, cr3
    retq

wrcr3:
    mov cr3, rdi
    retq

rdcr4:
    mov rax, cr4
    retq

wrcr4:
    mov cr4, rdi
    retq

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
retq

disable_caching:
    mov rax, cr0
    and eax, 0x8000000f
    mov cr0, rax
    retq

global wrmsr
global rdmsr

wrmsr:
    mov rcx, rdi
    mov rax, rsi
    mov rdx, rax
    shr rdx, 32
    wrmsr
    retq

rdmsr:
    mov rcx, rdi
    rdmsr
    shl rdx, 32
    or rdx, rax
    xchg rdx, rax
    retq

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
    retq

global loadidt
loadidt:
    lidt [rdi]
    retq

global loadtr
loadtr:
    ltr di
    retq

global invlpg
invlpg:
invlpg [rdi]
retq