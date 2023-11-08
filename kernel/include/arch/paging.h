#pragma once

#include <lib/stdint.h>
#include <lib/stddef.h>
#include <arch/x86_64/pml4.h>

extern void arch_unmap_full(void);
extern void arch_unmount(uintptr_t v);
extern int arch_pdiralloc(uintptr_t *ref);
extern void arch_fullvm_unmap(uintptr_t pml4);
extern int arch_mount(uintptr_t p, void **pvp);
extern void arch_unmap_n(uintptr_t v, size_t sz);
extern void arch_pagefree(uintptr_t v, size_t sz);
extern int arch_pagealloc(size_t sz, uintptr_t *addr);
extern void arch_unmap(int i4, int i3, int i2, int i1);
extern int arch_swtchvm(uintptr_t pdbr, uintptr_t *old);
extern int arch_getmapping(uintptr_t addr, pte_t **pte);
extern int arch_map_n(uintptr_t v, size_t sz, int flags);
extern int arch_memcpyvp(uintptr_t p, uintptr_t v, size_t size);
extern int arch_memcpypv(uintptr_t v, uintptr_t p, size_t size);
extern int arch_map_i(uintptr_t v, uintptr_t p, size_t sz, int flags);
extern int arch_memcpypp(uintptr_t pdst, uintptr_t psrc, size_t size);
extern int arch_map(uintptr_t frame, int i4, int i3, int i2, int i1, int flags);