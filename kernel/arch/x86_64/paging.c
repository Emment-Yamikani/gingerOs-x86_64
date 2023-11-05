#include <arch/cpu.h>
#include <arch/x86_64/pml4.h>
#include <bits/errno.h>
#include <mm/pmm.h>
#include <sys/thread.h>

#define I64_CLR(t) ({              \
    for (int i = 0; i < NPTE; ++i) \
        ((pte_t *)(t))[i].raw = 0; \
})

int i64_map(uintptr_t frame, int i4, int i3, int i2, int i1, int flags) {
    int err = -ENOMEM;
    uintptr_t l3 = 0, l2 = 0, l1 = 0;
    uintptr_t vaddr = __viraddr(i4, i3, i2, i1);

    if (isPS(flags))
        return -ENOTSUP;

    if (PML4E(i4)->p == 0) {
        if ((l3 = pmman.alloc()) == 0)
            goto error;

        PML4E(i4)->raw = l3 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PDPTE(i4, 0));
        I64_CLR(PDPTE(i4, 0));
    }

    if (PDPTE(i4, i3)->p == 0) {
        if ((l2 = pmman.alloc()) == 0)
            goto error;

        PDPTE(i4, i3)->raw = l2 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        I64_CLR(PDTE(i4, i3, 0));
    }

    if (PDTE(i4, i3, i2)->p == 0) {
        if ((l1 = pmman.alloc()) == 0)
            goto error;

        PDTE(i4, i3, i2)->raw = l1 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
        I64_CLR(PTE(i4, i3, i2, 0));
    }

    PTE(i4, i3, i2, i1)->raw = PGROUND(frame) | PGOFF(flags);
    invlpg(vaddr);
    return 0;
error:
    if (i1 != 0) {
        PDTE(i4, i3, i2)->raw = 0;
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
        pmman.free(l1);
    }

    if (l2 != 0) {
        PDPTE(i4, i3)->raw = 0;
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        pmman.free(l2);
    }

    if (l3 != 0) {
        PML4E(i4)->raw = 0;
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(l3);
    }

    return err;
}

int i64_unmap(int i4, int i3, int i2, int i1);