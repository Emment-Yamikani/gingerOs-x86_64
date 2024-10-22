#include <mm/pmm.h>
#include <mm/vmm.h>
#include <arch/cpu.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <mm/zone.h>
#include <sys/thread.h>
#include <arch/x86_64/ipi.h>
#include <arch/x86_64/paging.h>

#define x86_64_CLR(t) ({           \
    for (int i = 0; i < NPTE; ++i) \
        ((pte_t *)(t))[i].raw = 0; \
})

void x86_64_dumptable(pte_t *table) {
    printk("\t\t\t\t\t\t\t\t\tDUMP TABLE %p\n", table);
    for (int y = 0; y < 64; ++y) {
        printk("%2d: ", y);
        for (int x =0; x < 8; ++x)
            printk("%p |", table[y*8 + x]);
        printk("\n");
    }
}

static inline int x86_64_map_pdpt(int i4, int flags) {
    int        err  = 0;
    uintptr_t  l3 = 0;

    if (iL_INV(i4))
        return -EINVAL;

    if (_isPS(flags))
        return -ENOTSUP;

    if (!pte_isP(PML4E(i4))) {
        if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&l3))) {
            return err;
        }

        PML4E(i4)->raw = l3 | PGOFF(flags | PTE_WT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
    }

    return 0;
}

static inline int x86_64_map_pdt(int i4, int i3, int flags) {
    int         err     = -ENOMEM;
    uintptr_t   l3    = 0, l2 = 0;

    if (iL_INV(i4) || iL_INV(i3))
        return -EINVAL;

    if (_isPS(flags))
        return -ENOTSUP;

    if (!pte_isP(PML4E(i4))) {
        if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&l3))) {
            goto error;
        }

        PML4E(i4)->raw = l3 | PGOFF(flags | PTE_WT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
    }

    if (!pte_isP(PDPTE(i4, i3))) {
        if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&l2))) {
            goto error;
        }

        PDPTE(i4, i3)->raw = l2 | PGOFF(flags | PTE_WT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
    }

    return 0;
error:
    if (l3 != 0) {
        PML4E(i4)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(l3);
    }
    return err;
}

static inline int x86_64_map_pt(int i4, int i3, int i2, int flags) {
    int err = -ENOMEM;
    uintptr_t l3 = 0, l2 = 0, l1 = 0;

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2))
        return -EINVAL;

    if (_isPS(flags))
        return -ENOTSUP;

    if (!pte_isP(PML4E(i4))) {
        if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&l3))) {
            goto error;
        }

        PML4E(i4)->raw = l3 | PGOFF(flags | PTE_WT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
    }

    if (!pte_isP(PDPTE(i4, i3))) {
        if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&l2))) {
            goto error;
        }

        PDPTE(i4, i3)->raw = l2 | PGOFF(flags | PTE_WT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
    }

    if (!pte_isP(PDTE(i4, i3, i2))) {
        if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&l1))) {
            goto error;
        }

        PDTE(i4, i3, i2)->raw = l1 | PGOFF(flags | PTE_WT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PTE(i4, i3, i2, 0));
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
    }

    return 0;
error:
    if (l2 != 0) {
        PDPTE(i4, i3)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        pmman.free(l2);
    }

    if (l3 != 0) {
        PML4E(i4)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(l3);
    }
    return err;
}

static inline void x86_64_unmap_pdpt(int i4) {
    uintptr_t l3 = 0;

    if (iL_INV(i4))
        return;

    if (!pte_isP(PML4E(i4)))
        return;

    l3 = PGROUND(PML4E(i4)->raw);
    PML4E(i4)->raw = 0;
    send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
    invlpg((uintptr_t)PDPTE(i4, 0));
    pmman.free(l3);
}

static inline void x86_64_unmap_pdt(int i4, int i3) {
    uintptr_t l2 = 0;

    if (iL_INV(i4) || iL_INV(i3))
        return;

    if (!pte_isP(PML4E(i4)))
        return;
    
    if (!pte_isP(PDPTE(i4, i3)))
        return;

    l2 = PGROUND(PDPTE(i4, i3)->raw);
    PDPTE(i4, i3)->raw = 0;
    send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
    invlpg((uintptr_t)PDTE(i4, i3, 0));
    pmman.free(l2);
}

static inline void x86_64_unmap_pt(int i4, int i3, int i2) {
    uintptr_t l1 = 0;

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2))
        return;

    if (!pte_isP(PML4E(i4)))
        return;
    
    if (!pte_isP(PDPTE(i4, i3)))
        return;

    if (!pte_isP(PDTE(i4, i3, i2)))
        return;

    // printk("[%s:%d] i4: %d, i3: %d i2: %d\n", __FILE__, __LINE__, i4, i3, i2);
    l1 = PGROUND(PDTE(i4, i3, i2)->raw);
    PDTE(i4, i3, i2)->raw = 0;
    send_tlb_shootdown(rdcr3(), (uintptr_t)PTE(i4, i3, i2, 0));
    invlpg((uintptr_t)PTE(i4, i3, i2, 0));
    pmman.free(l1);
}

void x86_64_swtchvm(uintptr_t pdbr, uintptr_t *old) {
    if (old) *old = rdcr3();
    // if PDBR is null, then switch to the kernel address space (_PML4_)
    wrcr3(pdbr ? pdbr : VMA2LO(_PML4_));
}

int x86_64_map(uintptr_t pa, int i4, int i3, int i2, int i1, int flags) {
    int         err     = -ENOMEM;
    int         do_remap= _isremap(flags);
    uintptr_t   l3    = 0, l2 = 0, l1 = 0;
    uintptr_t   va   = i2v(i4, i3, i2, i1);

    flags   = extract_vmflags(flags);

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2) || iL_INV(i1))
        return -EINVAL;

    if (_isPS(flags))
        return -ENOTSUP;

    if (!pte_isP(PML4E(i4))) {
        if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&l3))) {
            goto error;
        }

        PML4E(i4)->raw = l3 | PGOFF(flags | PTE_WT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
        printk("%s:%d: l3: %p\n", __FILE__, __LINE__, l3);
    }

    if (!pte_isP(PDPTE(i4, i3))) {
        if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&l2))) {
            goto error;
        }

        PDPTE(i4, i3)->raw = l2 | PGOFF(flags | PTE_WT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        printk("%s:%d: l2: %p\n", __FILE__, __LINE__, l2);
    }

    if (!pte_isP(PDTE(i4, i3, i2))) {
        if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&l1))) {
            goto error;
        }

        PDTE(i4, i3, i2)->raw = l1 | PGOFF(flags | PTE_WT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PTE(i4, i3, i2, 0));
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
        printk("%s:%d: l1: %p\n", __FILE__, __LINE__, l1);
    }

    if (!pte_isP(PTE(i4, i3, i2, i1)))
        PTE(i4, i3, i2, i1)->raw = PGROUND(pa) | PGOFF(flags);
    else if (do_remap) // acknowledge remap request.
        PTE(i4, i3, i2, i1)->raw = PGROUND(pa) | PGOFF(flags);

    send_tlb_shootdown(rdcr3(), va);
    invlpg(va);

    assert(va != 0x1000000, "Reached\n");
    return 0;
error:
    if (l2 != 0) {
        PDPTE(i4, i3)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        pmman.free(l2);
    }

    if (l3 != 0) {
        PML4E(i4)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(l3);
    }

    return err;
}

void x86_64_unmap(int i4, int i3, int i2, int i1) {
    uintptr_t pa = 0;
    
    if (!pte_isP(PML4E(i4)))
        goto done;
    
    if (!pte_isP(PDPTE(i4, i3)))
        goto done;
    
    if (!pte_isP(PDTE(i4, i3, i2)))
        goto done;
    
    if (!pte_isP(PTE(i4, i3, i2, i1)))
        goto done;

    pa = PTE(i4, i3, i2, i1)->raw;
    PTE(i4, i3, i2, i1)->raw = 0;
    send_tlb_shootdown(rdcr3(), i2v(i4, i3, i2, i1));

done:
    invlpg(i2v(i4, i3, i2, i1));
    
    /** Deallocate this page frame
     * if it was allocated at the time of mapping.*/
    if (_isalloc(pa)) {
        printk("%s:%ld: %s: [NOTE]: Freeing page frame{0x%p}...\n", __FILE__, __LINE__, __func__, PGROUND(pa));
        pmman.free(PGROUND(pa));
    }
}

void x86_64_unmap_n(uintptr_t va, usize sz) {
    for (usize nr = NPAGE(sz); nr; --nr, va += PGSZ)
        x86_64_unmap(PML4I(va), PDPTI(va), PDI(va), PTI(va));
}

int x86_64_map_i(uintptr_t va, uintptr_t pa, usize sz, int flags) {
    int err = 0;
    uintptr_t vr = va;
    usize nr = NPAGE(sz);

    for (; nr; --nr, pa += PGSZ, va += PGSZ) {
        if ((err = x86_64_map(pa, PML4I(va),
            PDPTI(va), PDI(va), PTI(va), flags)))
            goto error;
    }

    return 0;
error:
    nr = NPAGE(sz) - nr;
    x86_64_unmap_n(vr, nr * PGSZ);
    return err;
}

int x86_64_mprotect(uintptr_t va, usize sz, int flags) {
    int     err     = 0;
    int     mask    = 0;
    pte_t   *pte    = NULL;
    usize  nr      = NPAGE(sz);
    va   = PGROUND(va);
    flags   = extract_vmflags(flags);

    if (_isP(flags) == 0)
        return -EINVAL;

    mask |= _isU(flags)   == 0 ? PTE_U : 0;
    mask |= _isR(flags)   == 0 ? PTE_R : 0;
    mask |= _isW(flags)   == 0 ? PTE_W : 0;
    mask |= _isX(flags)   == 0 ? PTE_X : 0;
    mask = extract_vmflags(mask);

    while (nr--) {
        if ((err = x86_64_getmapping(va, &pte)))
            return err;

        /**
         * @brief Mask out the page permissions we dont want
         */
        pte->raw &= ~mask; // Smart huh? ;)
        send_tlb_shootdown(rdcr3(), va);
        va += PGSZ;
    }

    return 0;
}

int x86_64_map_n(uintptr_t va, usize sz, int flags) {
    int         err     = 0;
    uintptr_t   pa      = 0;
    uintptr_t   vr      = va;
    usize       nr      = NPAGE(sz);
    gfp_t  gfp_mask     = GFP_NORMAL | (_iszero(flags) ? GFP_ZERO : 0);

    for (; nr; --nr, va += PGSZ) {
        if ((err = pmman.get_page(gfp_mask, (void **)&pa))) {
            goto error;
        }

        // printk("%s:%d: pa: %p\n", __FILE__, __LINE__, pa);

        if ((err = x86_64_map(pa, PML4I(va),
            PDPTI(va), PDI(va), PTI(va), flags | PTE_ALLOC)))
            goto error;
    }

    return 0;
error:
    nr = NPAGE(sz) - nr;
    x86_64_unmap_n(vr, nr * PGSZ);
    return err;
}

int x86_64_mount(uintptr_t pa, void **pvp) {
    int         err   = 0;
    uintptr_t   va = 0;

    if (pa == 0 || pvp == NULL)
        return -EINVAL;

    if ((va = vmman.alloc(PGSZ)) == 0)
        return -ENOMEM;
    
    if ((err = x86_64_map_i(va, pa, PGSZ, PTE_KRW)))
        goto error;

    *pvp = (void *)va;
    return 0;
error:
    if (va)
        vmman.free(va);
    return err;
}

void x86_64_unmount(uintptr_t va) {
    x86_64_unmap_n(va, PGSZ);
}

void x86_64_unmap_full(void) {
    usize i4 = 0, i3 = 0, i2 = 0, i1 = 0;
    for (i4 = 0; i4 < PML4I(USTACK); ++i4) {
        if (!pte_isP(PML4E(i4)))
            continue;
        for (i3 = 0; i3 < NPTE; ++i3) {
            if (!pte_isP(PDPTE(i4, i3)))
                continue;
            for (i2 = 0; i2 < NPTE; ++i2) {
                if (!pte_isP(PDTE(i4, i3, i2)))
                    continue;
                for (i1 = 0; i1 < NPTE; ++i1) {
                    if (!pte_isP(PTE(i4, i3, i2, i1)))
                        continue;
                    x86_64_unmap(i4, i3, i2, i1);
                }
                x86_64_unmap_pt(i4, i3, i2);
            }
            x86_64_unmap_pdt(i4, i3);
        }
        x86_64_unmap_pdpt(i4);
    }
}

void x86_64_fullvm_unmap(uintptr_t pml4) {
    uintptr_t oldpml4 = 0;

    if (pml4 == 0)
        return;
    x86_64_swtchvm(pml4, &oldpml4);
    x86_64_unmap_full();
    x86_64_swtchvm(oldpml4, NULL);
}

static int x86_64_kvmcpy(uintptr_t dstp) {
    int     err     = 0;
    pte_t   *dstv   = NULL;

    if ((dstp == 0) || PGOFF(dstp))
        return -EINVAL;
    
    if ((err = x86_64_mount(dstp, (void **)&dstv)))
        return err;

    /// TODO: lock higer vmmap and kernel pDBr.
    for (usize i = PML4I(USTACK); i < NPTE; ++i)
        dstv[i] = PML4[i];
    
    dstv[510].raw = dstp | PTE_KRW | PTE_WTCD;
    /// TODO: unlock higer vmmap and kernel pDBr.

    x86_64_unmount((uintptr_t)dstv);
    return 0;
}

int x86_64_lazycpy(uintptr_t dst, uintptr_t src) {
    int         err     = 0;
    uintptr_t   oldpdbr = 0;
    usize       i4      = 0, i3 = 0, i2 = 0, i1 = 0;
    pte_t       *pml4   = NULL, *pdpt = NULL, *pdt = NULL, *pt = NULL;

    if (dst == 0 || src == 0)
        return -EINVAL;

    /**
     * mount the source root page table
     * this is the virtual address space
     * we want to perform a copy from.
    */
    if ((err = x86_64_mount(src, (void **)&pml4)))
        return err;
    
    x86_64_swtchvm(dst, &oldpdbr);
    
    /**
     * Begin the process of copying the various table entries.
     * this for copies only the currently mapped PDPT/PML4
     * from PML4e[0] -> PML4e[255], i.e. from 0x0 -> USTACK.
    */
    for (i4 = 0; i4 < PML4I(USTACK); ++i4) {
        if (!pte_isP(&pml4[i4]))
            continue;

        // Map this PDPT into the destination PML4.
        if ((err = x86_64_map_pdpt(i4, PGOFF(pml4[i4].raw))))
            goto error;

        // create a temporal mount for this PDPT.
        if ((err = x86_64_mount(pml4[i4].raw, (void **)&pdpt)))
            goto error;

        /**
         * Begin the process of copying the various table entries.
         * this for copies only the currently mapped PDT/PDPT.
         */
        for (i3 = 0; i3 < NPTE; ++i3) {
            if (!pte_isP(&pdpt[i3]))
                continue;

            // Map this PDT into the destination PDPT.
            if ((err = x86_64_map_pdt(i4, i3, PGOFF(pdpt[i3].raw)))) {
                x86_64_unmount((uintptr_t)pdpt);
                goto error;
            }

            // create a temporal mount for this PDT.
            if ((err = x86_64_mount(pdpt[i3].raw, (void **)&pdt))) {
                x86_64_unmount((uintptr_t)pdpt);
                goto error;
            }

            /**
             * Begin the process of copying the various table entries.
             * this for copies only the currently mapped PT/PDT
             */
            for (i2 = 0; i2 < NPTE; ++i2) {
                if (!pte_isP(&pdt[i2]))
                    continue;

                // Map this PT into the destination PDT.
                if ((err = x86_64_map_pt(i4, i3, i2, PGOFF(pdt[i2].raw)))) {
                    x86_64_unmount((uintptr_t)pdt);
                    x86_64_unmount((uintptr_t)pdpt);
                    goto error;
                }

                // create a temporal mount for this PT.
                if ((err = x86_64_mount(pdt[i2].raw, (void **)&pt))) {
                    x86_64_unmount((uintptr_t)pdt);
                    x86_64_unmount((uintptr_t)pdpt);
                    goto error;
                }

                /**
                 * Begin the process of copying the various table entries.
                 * this 'for' copies only the currently mapped pages/PT
                 */
                for (i1 = 0; i1 < NPTE; ++i1) {
                    if (!pte_isP(&pt[i1]))
                        continue;

                    // Enforce Copy-On-Write by marking page ready-only.
                    if (pte_isW(&pt[i1])) {
                        pt[i1].w = 0;
                        send_tlb_shootdown(rdcr3(), i2v(i4, i3, i2, i1));
                    }
                    
                    // Do page copy.
                    PTE(i4, i3, i2, i1)->raw = pt[i1].raw;

                    // No need of incrementing MMIO address-space pages.                    
                    if (ismmio_addr(PGROUND(pt[i1].raw)))
                        continue;

                    // increase the page count on this page.
                    if ((err = __page_getref(PGROUND(pt[i1].raw)))) {
                        assert(0, "Failed to increament page ref.");
                        x86_64_unmount((uintptr_t)pdt);
                        x86_64_unmount((uintptr_t)pdpt);
                        goto error;
                    }
                }

                x86_64_unmount((uintptr_t)pt);
            }

            x86_64_unmount((uintptr_t)pdt);
        }

        x86_64_unmount((uintptr_t)pdpt);
    }

    x86_64_swtchvm(oldpdbr, NULL);
    x86_64_unmount((uintptr_t)pml4);
    return 0;
error:
    x86_64_unmap_full();
    x86_64_swtchvm(oldpdbr, NULL);
    x86_64_unmount((uintptr_t)pml4);
    return err;
}

int x86_64_memcpypp(uintptr_t pdst, uintptr_t psrc, usize size) {
    int         err     = 0;
    usize       len     = 0;
    uintptr_t   vdst    = 0, vsrc = 0;

    for (; size; size -= len, psrc += len, pdst += len) {
        if ((err = x86_64_mount(PGROUND(pdst), (void **)&vdst)))
            return err;

        if ((err = x86_64_mount(PGROUND(psrc), (void **)&vsrc))) {
            x86_64_unmount(vdst);
            return err;
        }

        len = MIN(PAGESZ - MAX(PGOFF(psrc), PGOFF(pdst)), PAGESZ);
        len = MIN(len, size);
        memcpy((void *)(vdst + PGOFF(pdst)), (void *)(vsrc + PGOFF(psrc)), len);

        x86_64_unmount(vsrc);
        x86_64_unmount(vdst);
    }

    return 0;
}

int x86_64_memcpyvp(uintptr_t pa, uintptr_t va, usize size) {
    int         err     = 0;
    usize       len     = 0;
    uintptr_t   vdst    = 0;
    
    for (; size; size -= len, pa += len, va += len) {
        if ((err = x86_64_mount(PGROUND(pa), (void **)&vdst)))
            return err;

        len = MIN(PAGESZ - PGOFF(pa), size);
        memcpy((void *)(vdst + PGOFF(pa)), (void *)va, len);
        x86_64_unmount(vdst);
    }

    return 0;
}

int x86_64_memcpypv(uintptr_t va, uintptr_t pa, usize size) {
    int         err     = 0;
    usize       len     = 0;
    uintptr_t   vsrc    = 0;
    
    for (; size; size -= len, pa += len, va += len) {
        if ((err = x86_64_mount(PGROUND(pa), (void **)&vsrc)))
            return err;

        len = MIN(PAGESZ - PGOFF(pa), size);
        memcpy((void *)va, (void *)(vsrc + PGOFF(pa)), len);
        x86_64_unmount(vsrc);
    }

    return 0;
}

int x86_64_getmapping(uintptr_t addr, pte_t **pte) {
    int i4 = PML4I(addr),
        i3 = PDPTI(addr),
        i2 = PDI(addr),
        i1 = PTI(addr);

    if (addr == 0)
        return -EINVAL;

    if (!pte_isP(PML4E(i4)))
        return -ENOENT;
    
    if (!pte_isP(PDPTE(i4, i3)))
        return -ENOENT;
    
    if (!pte_isP(PDTE(i4, i3, i2)))
        return -ENOENT;
    
    if (!pte_isP(PTE(i4, i3, i2, i1)))
        return -ENOENT;
    
    if (pte) {
        *pte = PTE(i4, i3, i2, i1);
    }
    return 0;
}

int x86_64_pml4alloc(uintptr_t *ref) {
    int         err     = 0;
    uintptr_t   pml4    = 0;

    if (ref == NULL)
        return -EINVAL;
    
    if ((err = pmman.get_page(GFP_NORMAL | GFP_ZERO, (void **)&pml4))) {
        return err;
    }
    
    if ((err = x86_64_kvmcpy(pml4)))
        goto error;

    *ref = pml4;
    return 0;
error:
    pmman.free(pml4);
    return err;
}

void x86_64_pml4free(uintptr_t pgdir) {
    if (pgdir)
        pmman.free(pgdir);
}