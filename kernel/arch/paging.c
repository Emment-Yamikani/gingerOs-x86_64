#include <arch/x86_64/pml4.h>
#include <arch/x86_64/cpu.h>
#include <mm/vmm.h>
#include <bits/errno.h>

int arch_swtchvm(uintptr_t pdbr, uintptr_t *old) {
#if defined (__x86_64__)
    return i64_swtchvm(pdbr, old);
#endif
}

int arch_map(uintptr_t frame, int i4, int i3, int i2, int i1, int flags) {
#if defined (__x86_64__)
    return i64_map(frame, i4, i3, i2, i1, flags);
#endif
}

void arch_unmap(int i4, int i3, int i2, int i1) {
#if defined (__x86_64__)
    return i64_unmap(i4, i3, i2, i1);
#endif
}

void arch_unmap_n(uintptr_t v, size_t sz) {
#if defined (__x86_64__)
    return i64_unmap_n(v, sz);
#endif
}

int arch_map_i(uintptr_t v, uintptr_t p, size_t sz, int flags) {
#if defined (__x86_64__)
    return i64_map_i(v, p, sz, flags);
#endif
}

int arch_map_n(uintptr_t v, size_t sz, int flags) {
#if defined (__x86_64__)
    return i64_map_n(v, sz, flags);
#endif
}

int arch_mount(uintptr_t p, void **pvp) {
#if defined (__x86_64__)
    return i64_mount(p, pvp);
#endif
}

void arch_unmount(uintptr_t v) {
#if defined (__x86_64__)
    return i64_unmount(v);
#endif
}

void arch_unmap_full(void) {
#if defined (__x86_64__)
    return i64_unmap_full();
#endif
}

void arch_fullvm_unmap(uintptr_t pml4) {
#if defined (__x86_64__)
    return i64_fullvm_unmap(pml4);
#endif
}

int arch_lazycpy(uintptr_t dst, uintptr_t src) {
#if defined (__x86_64__)
    return i64_lazycpy(dst, src);
#endif
}

int arch_memcpypp(uintptr_t pdst, uintptr_t psrc, size_t size) {
#if defined (__x86_64__)
    return i64_memcpypp(pdst, psrc, size);
#endif
}

int arch_memcpyvp(uintptr_t p, uintptr_t v, size_t size) {
#if defined (__x86_64__)
    return i64_memcpyvp(p, v, size);
#endif
}

int arch_memcpypv(uintptr_t v, uintptr_t p, size_t size) {
#if defined (__x86_64__)
    return i64_memcpypv(v, p, size);
#endif
}

int arch_getmapping(uintptr_t addr, pte_t **pte) {
#if defined (__x86_64__)
    return i64_getmapping(addr, pte);
#endif
}

int arch_pdiralloc(uintptr_t *ref) {
#if defined (__x86_64__)
    return i64_pml4alloc(ref);
#endif
}

int arch_pagealloc(size_t sz, uintptr_t *addr) {
    int err = 0;
    uintptr_t v = 0;

    if (addr == NULL)
        return -EINVAL;

    if ((v = vmman.alloc(sz)) == 0)
        return -ENOMEM;
    

    if ((err = arch_map_n(v, sz, VM_KRW))) {
        debugloc();
        vmman.free(v);
        return err;
    }

    *addr = v;
    return 0;
}

void arch_pagefree(uintptr_t v, size_t sz) {
    arch_unmap_n(v, sz);
    vmman.free(v);
}