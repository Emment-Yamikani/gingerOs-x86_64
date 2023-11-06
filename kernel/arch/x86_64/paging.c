#include <arch/cpu.h>
#include <arch/x86_64/pml4.h>
#include <bits/errno.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/thread.h>
#include <mm/mm_zone.h>

#define I64_CLR(t) ({              \
    for (int i = 0; i < NPTE; ++i) \
        ((pte_t *)(t))[i].raw = 0; \
})

static inline int i64_map_pdpt(int i4, int flags) {
    uintptr_t lvl3 = 0;

    if (iL_INV(i4))
        return -EINVAL;

    if (isPS(flags))
        return -ENOTSUP;

    if (PML4E(i4)->p == 0) {
        if ((lvl3 = pmman.alloc()) == 0)
            return -ENOMEM;

        PML4E(i4)->raw = lvl3 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PDPTE(i4, 0));
        I64_CLR(PDPTE(i4, 0));
    }

    return 0;
}

static inline int i64_map_pdt(int i4, int i3, int flags) {
    int err = 0;
    uintptr_t lvl3 = 0, lvl2 = 0;

    if (iL_INV(i4) || iL_INV(i3))
        return -EINVAL;
    
    if (isPS(flags))
        return -ENOTSUP;

    if (PML4E(i4)->p == 0) {
        if ((lvl3 = pmman.alloc()) == 0)
            goto error;

        PML4E(i4)->raw = lvl3 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PDPTE(i4, 0));
        I64_CLR(PDPTE(i4, 0));
    }

    if (PDPTE(i4, i3)->p == 0) {
        if ((lvl2 = pmman.alloc()) == 0)
            goto error;

        PDPTE(i4, i3)->raw = lvl2 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        I64_CLR(PDTE(i4, i3, 0));
    }

    return 0;
error:
    if (lvl3 != 0) {
        PML4E(i4)->raw = 0;
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(lvl3);
    }
    return err;
}

static inline int i64_map_pt(int i4, int i3, int i2, int flags) {
    int err = 0;
    uintptr_t lvl3 = 0, lvl2 = 0, lvl1 = 0;

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2))
        return -EINVAL;

    if (isPS(flags))
        return -ENOTSUP;

    if (PML4E(i4)->p == 0) {
        if ((lvl3 = pmman.alloc()) == 0)
            goto error;

        PML4E(i4)->raw = lvl3 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PDPTE(i4, 0));
        I64_CLR(PDPTE(i4, 0));
    }

    if (PDPTE(i4, i3)->p == 0) {
        if ((lvl2 = pmman.alloc()) == 0)
            goto error;

        PDPTE(i4, i3)->raw = lvl2 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        I64_CLR(PDTE(i4, i3, 0));
    }

    if (PDTE(i4, i3, i2)->p == 0) {
        if ((lvl1 = pmman.alloc()) == 0)
            goto error;

        PDTE(i4, i3, i2)->raw = lvl1 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
        I64_CLR(PTE(i4, i3, i2, 0));
    }

    return 0;
error:
    if (lvl2 != 0) {
        PDPTE(i4, i3)->raw = 0;
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        pmman.free(lvl2);
    }

    if (lvl3 != 0) {
        PML4E(i4)->raw = 0;
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(lvl3);
    }
    return err;
}

static inline void i64_unmap_pdpt(int i4) {
    uintptr_t lvl3 = 0;

    if (iL_INV(i4))
        return;

    if (ispresent(PML4E(i4)->raw)) {
        lvl3 = PGROUND(PML4E(i4)->raw);
        PML4E(i4)->raw = 0;
        invlpg((uintptr_t)PDPTE(i4, 0));

        //send_tlbshootdown();
        pmman.free(lvl3);
    }
}

static inline void i64_unmap_pdt(int i4, int i3) {
    uintptr_t lvl2 = 0;

    if (iL_INV(i4) || iL_INV(i3))
        return;

    if (!ispresent(PML4E(i4)->raw))
        return;
    
    if (ispresent(PDPTE(i4, i3)->raw)) {
        lvl2 = PGROUND(PDPTE(i4, i3)->raw);
        PDPTE(i4, i3)->raw = 0;
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        // send_tlbshootdown();
        pmman.free(lvl2);
    }
}

static inline void i64_unmap_pt(int i4, int i3, int i2) {
    uintptr_t lvl1 = 0;

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2))
        return;

    if (!ispresent(PML4E(i4)->raw))
        return;
    
    if (!ispresent(PDPTE(i4, i3)->raw))
        return;
    
    if (ispresent(PDTE(i4, i3, i2)->raw)) {
        lvl1 = PGROUND(PDTE(i4, i3, i2)->raw);
        PDTE(i4, i3, i2)->raw = 0;
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
        // send_tlbshootdown();
        pmman.free(lvl1);
    }
}

int i64_swtchvm(uintptr_t pdbr, uintptr_t *old) {
    if (pdbr == 0)
        return -EINVAL;
    if (old)
        *old = rdcr3();
    wrcr3(pdbr);
    return 0;
}

int i64_map(uintptr_t frame, int i4, int i3, int i2, int i1, int flags) {
    int err = -ENOMEM;
    uintptr_t lvl3 = 0, lvl2 = 0, lvl1 = 0;
    uintptr_t vaddr = __viraddr(i4, i3, i2, i1);

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2) || iL_INV(i1))
        return -EINVAL;

    if (isPS(flags))
        return -ENOTSUP;

    if (PML4E(i4)->p == 0) {
        if ((lvl3 = pmman.alloc()) == 0)
            goto error;

        PML4E(i4)->raw = lvl3 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PDPTE(i4, 0));
        I64_CLR(PDPTE(i4, 0));
    }

    if (PDPTE(i4, i3)->p == 0) {
        if ((lvl2 = pmman.alloc()) == 0)
            goto error;

        PDPTE(i4, i3)->raw = lvl2 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        I64_CLR(PDTE(i4, i3, 0));
    }

    if (PDTE(i4, i3, i2)->p == 0) {
        if ((lvl1 = pmman.alloc()) == 0)
            goto error;

        PDTE(i4, i3, i2)->raw = lvl1 | PGOFF(flags | VM_PWT | VM_KRW);
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
        I64_CLR(PTE(i4, i3, i2, 0));
    }

    if (PTE(i4, i3, i2, i1)->p == 0)
        PTE(i4, i3, i2, i1)->raw = PGROUND(frame) | PGOFF(flags);
    invlpg(vaddr);
    return 0;
error:
    if (lvl2 != 0) {
        PDPTE(i4, i3)->raw = 0;
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        pmman.free(lvl2);
    }

    if (lvl3 != 0) {
        PML4E(i4)->raw = 0;
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(lvl3);
    }

    return err;
}

void i64_unmap(int i4, int i3, int i2, int i1) {
    if (PML4E(i4)->p == 0)
        return;
    
    if (!ispresent(PDPTE(i4, i3)->raw))
        return;
    
    if (!ispresent(PDTE(i4, i3, i2)->raw))
        return;
    
    if (!ispresent(PTE(i4, i3, i2, i1)->raw))
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

void i64_unmap_full(void) {
    size_t i4 = 0, i3 = 0, i2 = 0, i1 = 0;
    for (i4 = 0; i4 < PML4I(USTACK); ++i4) {
        for (i3 = 0; i3 < NPTE; ++i3) {
            for (i2 = 0; i2 < NPTE; ++i2) {
                for (i1 = 0; i1 < NPTE; ++i1) {
                    i64_unmap(i4, i3, i2, i1);
                }
                i64_unmap_pt(i4, i3, i2);
            }
            i64_unmap_pdt(i4, i3);
        }
        i64_unmap_pdpt(i4);
    }
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
    uintptr_t oldpdbr = 0;
    size_t i4 = 0, i3 = 0, i2 = 0, i1 = 0;
    pte_t *pml4 = NULL, *pdpt = NULL, *pdt = NULL, *pt = NULL;

    /**
     * mount the source root page table
     * this is the virtual address space
     * we want to perform a copy from.
    */
    if ((err = i64_mount(src, &pml4)))
        return err;
    
    if ((err = i64_swtchvm(dst, &oldpdbr))) {
        i64_unmount((uintptr_t)pml4);
        return err;
    }
    
    /**
     * Begin the process of copying the various table entries.
     * this for copies only the currently mapped PDPT/PML4
     * from PML4e[0] -> PML4e[255], i.e. from 0x0 -> USTACK.
    */
    for (i4 = 0; i4 < PML4I(USTACK); ++i4) {
        if (!ispresent(pml4[i4].raw))
            continue;

        // Map this PDPT into the destination PML4.
        if ((err = i64_map_pdpt(i4, PGOFF(pml4[i4].raw))))
            goto error;

        // create a temporal mount for this PDPT.
        if ((err = i64_mount(pml4[i4].raw, &pdpt)))
            goto error;

        /**
         * Begin the process of copying the various table entries.
         * this for copies only the currently mapped PDT/PDPT.
         */
        for (i3 = 0; i3 < NPTE; ++i3) {
            if (!ispresent(pdpt[i3].raw))
                continue;

            // Map this PDT into the destination PDPT.
            if ((err = i64_map_pdt(i4, i3, PGOFF(pdpt[i3].raw)))) {
                i64_unmount((uintptr_t)pdpt);
                goto error;
            }

            // create a temporal mount for this PDT.
            if ((err = i64_mount(pdpt[i3].raw, &pdt))) {
                i64_unmount((uintptr_t)pdpt);
                goto error;
            }

            /**
             * Begin the process of copying the various table entries.
             * this for copies only the currently mapped PT/PDT
             */
            for (i2 = 0; i2 < NPTE; ++i2) {
                if (!ispresent(pdt[i2].raw))
                    continue;

                // Map this PT into the destination PDT.
                if ((err = i64_map_pt(i4, i3, i2, PGOFF(pdt[i2].raw)))) {
                    i64_unmount((uintptr_t)pdt);
                    i64_unmount((uintptr_t)pdpt);
                    goto error;
                }

                // create a temporal mount for this PT.
                if ((err = i64_mount(pdt[i2].raw, &pt))) {
                    i64_unmount((uintptr_t)pdt);
                    i64_unmount((uintptr_t)pdpt);
                    goto error;
                }

                /**
                 * Begin the process of copying the various table entries.
                 * this for copies only the currently mapped pages/PT
                 */
                for (i1 = 0; i1 < NPTE; ++i1) {
                    if (!ispresent(pt[i1].raw))
                        continue;

                    // Enforce Copy-On-Write by marking page ready-only.
                    if (iswritable(pt[i1].raw)) {
                        pt[i1].w = 0;
                        //send_tlbshootdown();
                    }
                    
                    // Do page copy.
                    PTE(i4, i3, i2, i1)->raw = pt[i1].raw;
                    
                    // increase the page count on this page.
                    if ((err = __page_incr(PGROUND(pt[i1].raw)))) {
                        i64_unmount((uintptr_t)pt);
                        i64_unmount((uintptr_t)pdt);
                        i64_unmount((uintptr_t)pdpt);
                        goto error;
                    }
                }

                i64_unmount((uintptr_t)pt);
            }

            i64_unmount((uintptr_t)pdt);
        }

        i64_unmount((uintptr_t)pdpt);
    }

    i64_swtchvm(oldpdbr, NULL);
    i64_unmount((uintptr_t)pml4);
    return 0;
error:
    i64_unmap_full();
    i64_swtchvm(oldpdbr, NULL);
    i64_unmount((uintptr_t)pml4);
    return err;
}