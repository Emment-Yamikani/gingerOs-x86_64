#include <arch/cpu.h>
#include <arch/x86_64/pml4.h>
#include <bits/errno.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
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

    if (PTE(i4, i3, i2, i1)->p == 0)
        PTE(i4, i3, i2, i1)->raw = PGROUND(frame) | PGOFF(flags);
    invlpg(vaddr);
    return 0;
error:
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

void i64_unmap(int i4, int i3, int i2, int i1) {
    if (PML4E(i4)->p == 0)
        return;
    
    if (PDPTE(i4, i3)->p == 0)
        return;
    
    if (PDTE(i4, i3, i2)->p == 0)
        return;
    
    if (PTE(i4, i3, i2, i1)->p == 0)
        return;
    
    PTE(i4, i3, i2, i1)->raw = 0;
    invlpg(__viraddr(i4, i3, i2, i1));
}

void i64_unmap_n(uintptr_t v, size_t sz) {
    for (size_t nr = NPAGE(sz); nr; --nr, v += PGSZ)
        i64_unmap(PML4I(v), PDPTI(v), PDI(v), PTI(v));
}

int i64_map_i(uintptr_t v, uintptr_t p, size_t sz, int flags) {
    int err = 0;
    uintptr_t vr = v;
    size_t nr = NPAGE(sz);

    for (; nr; --nr, p += PGSZ, v += PGSZ) {
        if ((err = i64_map(p, PML4I(v), PDPTI(v), PDI(v), PTI(v), flags)))
            goto error;
    }

    return 0;
error:
    nr = NPAGE(sz) - nr;
    i64_unmap_n(vr, nr * PGSZ);
    return err;
}

int i64_map_n(uintptr_t v, size_t sz, int flags) {
    int err = 0;
    uintptr_t p = 0;
    uintptr_t vr = v;
    size_t nr = NPAGE(sz);

    for (; nr; --nr, v += PGSZ) {
        if ((p = pmman.alloc()) == 0) {
            err = -ENOMEM;
            goto error;
        }

        if ((err = i64_map(p, PML4I(v),
                           PDPTI(v), PDI(v), PTI(v), flags)))
            goto error;
    }

    return 0;
error:
    nr = NPAGE(sz) - nr;
    i64_unmap_n(vr, nr * PGSZ);
    return err;
}

int i64_mount(uintptr_t p, void **pvp) {
    int err = 0;
    uintptr_t v = 0;

    if (p == 0 || pvp == NULL)
        return -EINVAL;

    if ((v = vmman.alloc(PGSZ)) == 0)
        return -ENOMEM;
    
    if ((err = i64_map_i(v, p, PGSZ, VM_KRW)))
        goto error;

    *pvp = v;
    return 0;
error:
    if (v)
        vmman.free(v);
    return err;
}

void i64_unmount(uintptr_t v) {
    i64_unmap_n(v, PGSZ);
}

int i64_kvmcpy(uintptr_t dstp) {
    int err = 0;
    pte_t *dstv = NULL;

    if (dstp == NULL || PGOFF(dstp))
        return -EINVAL;
    
    if ((err = i64_mount(dstp, &dstv)))
        return err;
    
    for (size_t i = PML4I(USTACK); i < NPTE; ++i)
        dstv[i] = PML4[i];

    i64_unmount((uintptr_t)dstv);
    return 0;
}

int i64_lazycpy(uintptr_t dst, uintptr_t src) {
    int err = 0;
    pte_t *srcv = NULL;
    size_t i4 = 0, i3 = 0, i2 = 0, i1 = 0;

    if ((err = i64_mount(src, &srcv)))
        goto error;
    
    for (i4 = 0; i4 < PML4I(USTACK); ++i4) {
        for (i3 = 0; i3 < NPTE; ++i3) {
            for (i2 = 0; i2 < NPTE; ++i2) {
                for (i1 = 0; i1 < NPTE; ++i1) {
                }
            }
        }
    }

    return 0;
error:
    if (srcv)
        i64_unmount((uintptr_t)srcv);
    return err;
}