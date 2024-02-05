#include <arch/cpu.h>
#include <arch/x86_64/ipi.h>
#include <arch/x86_64/paging.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <mm/mm_zone.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/thread.h>

spinlock_t kmap_lk = SPINLOCK_INIT();
pagemap_t kernel_map = {
    .flags = 0,
    .lock = SPINLOCK_INIT(),
    .pdbr = (void *)VMA2LO(_PML4_),
};

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

spinlock_t *kvmhigh_lock = SPINLOCK_NEW();

static inline int x86_64_map_pdpt(int i4, int flags) {
    uintptr_t lvl3 = 0;

    if (iL_INV(i4))
        return -EINVAL;

    if (_isPS(flags))
        return -ENOTSUP;

    if (!pte_ispresent(PML4E(i4))) {
        if ((lvl3 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
            return -ENOMEM;

        PML4E(i4)->raw = lvl3 | PGOFF(flags | PTE_PWT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
    }

    return 0;
}

static inline int x86_64_map_pdt(int i4, int i3, int flags) {
    int err = -ENOMEM;
    uintptr_t lvl3 = 0, lvl2 = 0;

    if (iL_INV(i4) || iL_INV(i3))
        return -EINVAL;
    
    if (_isPS(flags))
        return -ENOTSUP;

    if (!pte_ispresent(PML4E(i4))) {
        if ((lvl3 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
            goto error;

        PML4E(i4)->raw = lvl3 | PGOFF(flags | PTE_PWT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
    }

    if (!pte_ispresent(PDPTE(i4, i3))) {
        if ((lvl2 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
            goto error;

        PDPTE(i4, i3)->raw = lvl2 | PGOFF(flags | PTE_PWT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
    }

    return 0;
error:
    if (lvl3 != 0) {
        PML4E(i4)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(lvl3);
    }
    return err;
}

static inline int x86_64_map_pt(int i4, int i3, int i2, int flags) {
    int err = -ENOMEM;
    uintptr_t lvl3 = 0, lvl2 = 0, lvl1 = 0;

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2))
        return -EINVAL;

    if (_isPS(flags))
        return -ENOTSUP;

    if (!pte_ispresent(PML4E(i4))) {
        if ((lvl3 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
            goto error;

        PML4E(i4)->raw = lvl3 | PGOFF(flags | PTE_PWT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
    }

    if (!pte_ispresent(PDPTE(i4, i3))) {
        if ((lvl2 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
            goto error;

        PDPTE(i4, i3)->raw = lvl2 | PGOFF(flags | PTE_PWT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
    }

    if (!pte_ispresent(PDTE(i4, i3, i2))) {
        if ((lvl1 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
            goto error;

        PDTE(i4, i3, i2)->raw = lvl1 | PGOFF(flags | PTE_PWT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PTE(i4, i3, i2, 0));
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
    }

    return 0;
error:
    if (lvl2 != 0) {
        PDPTE(i4, i3)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        pmman.free(lvl2);
    }

    if (lvl3 != 0) {
        PML4E(i4)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(lvl3);
    }
    return err;
}

static inline void x86_64_unmap_pdpt(int i4) {
    uintptr_t lvl3 = 0;

    if (iL_INV(i4))
        return;

    if (!pte_ispresent(PML4E(i4)))
        return;

    lvl3 = PGROUND(PML4E(i4)->raw);
    PML4E(i4)->raw = 0;
    send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
    invlpg((uintptr_t)PDPTE(i4, 0));
    pmman.free(lvl3);
}

static inline void x86_64_unmap_pdt(int i4, int i3) {
    uintptr_t lvl2 = 0;

    if (iL_INV(i4) || iL_INV(i3))
        return;

    if (!pte_ispresent(PML4E(i4)))
        return;
    
    if (!pte_ispresent(PDPTE(i4, i3)))
        return;

    lvl2 = PGROUND(PDPTE(i4, i3)->raw);
    PDPTE(i4, i3)->raw = 0;
    send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
    invlpg((uintptr_t)PDTE(i4, i3, 0));
    pmman.free(lvl2);
}

static inline void x86_64_unmap_pt(int i4, int i3, int i2) {
    uintptr_t lvl1 = 0;

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2))
        return;

    if (!pte_ispresent(PML4E(i4)))
        return;
    
    if (!pte_ispresent(PDPTE(i4, i3)))
        return;

    if (!pte_ispresent(PDTE(i4, i3, i2)))
        return;

    // printk("[%s:%d] i4: %d, i3: %d i2: %d\n", __FILE__, __LINE__, i4, i3, i2);
    lvl1 = PGROUND(PDTE(i4, i3, i2)->raw);
    PDTE(i4, i3, i2)->raw = 0;
    send_tlb_shootdown(rdcr3(), (uintptr_t)PTE(i4, i3, i2, 0));
    invlpg((uintptr_t)PTE(i4, i3, i2, 0));
    pmman.free(lvl1);
}

void x86_64_swtchvm(uintptr_t pdbr, uintptr_t *old) {
    if (old)
        *old = rdcr3();
    // if PDBR is null, then switch to the kernel address space (_PML4_)
    wrcr3(pdbr ? pdbr : VMA2LO(_PML4_));
}

int x86_64_map(uintptr_t paddr, int i4, int i3, int i2, int i1, int flags) {
    int         err     = -ENOMEM;
    int         do_remap= _isremap(flags);
    uintptr_t   lvl3    = 0, lvl2 = 0, lvl1 = 0;
    uintptr_t   vaddr   = __viraddr(i4, i3, i2, i1);

    flags   = extract_vmflags(flags);

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2) || iL_INV(i1))
        return -EINVAL;

    if (_isPS(flags))
        return -ENOTSUP;

    if (!pte_ispresent(PML4E(i4))) {
        if ((lvl3 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
            goto error;

        PML4E(i4)->raw = lvl3 | PGOFF(flags | PTE_PWT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
    }

    if (!pte_ispresent(PDPTE(i4, i3))) {
        if ((lvl2 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
            goto error;

        PDPTE(i4, i3)->raw = lvl2 | PGOFF(flags | PTE_PWT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
    }

    if (!pte_ispresent(PDTE(i4, i3, i2))) {
        if ((lvl1 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
            goto error;

        PDTE(i4, i3, i2)->raw = lvl1 | PGOFF(flags | PTE_PWT | PTE_KRW);
        send_tlb_shootdown(rdcr3(), (uintptr_t)PTE(i4, i3, i2, 0));
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
    }

    if (!pte_ispresent(PTE(i4, i3, i2, i1)))
        PTE(i4, i3, i2, i1)->raw = PGROUND(paddr) | PGOFF(flags);
    else if (do_remap) // acknowledge remap request.
        PTE(i4, i3, i2, i1)->raw = PGROUND(paddr) | PGOFF(flags);

    send_tlb_shootdown(rdcr3(), vaddr);
    invlpg(vaddr);
    return 0;
error:
    if (lvl2 != 0) {
        PDPTE(i4, i3)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDTE(i4, i3, 0));
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        pmman.free(lvl2);
    }

    if (lvl3 != 0) {
        PML4E(i4)->raw = 0;
        send_tlb_shootdown(rdcr3(), (uintptr_t)PDPTE(i4, 0));
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(lvl3);
    }

    return err;
}

void x86_64_unmap(int i4, int i3, int i2, int i1) {
    uintptr_t paddr = 0;
    
    if (!pte_ispresent(PML4E(i4)))
        goto done;
    
    if (!pte_ispresent(PDPTE(i4, i3)))
        goto done;
    
    if (!pte_ispresent(PDTE(i4, i3, i2)))
        goto done;
    
    if (!pte_ispresent(PTE(i4, i3, i2, i1)))
        goto done;

    paddr = PTE(i4, i3, i2, i1)->raw;
    PTE(i4, i3, i2, i1)->raw = 0;
    send_tlb_shootdown(rdcr3(), __viraddr(i4, i3, i2, i1));

done:
    invlpg(__viraddr(i4, i3, i2, i1));
    
    /** Deallocate this page frame
     * if it was allocated at the time of mapping.*/
    if (_isalloc_page(paddr)) {
        printk("%s:%ld: %s: [NOTE]: Freeing page frame{0x%p}...\n", __FILE__, __LINE__, __func__, PGROUND(paddr));
        pmman.free(PGROUND(paddr));
    }
}

void x86_64_unmap_n(uintptr_t vaddr, size_t sz) {
    for (size_t nr = NPAGE(sz); nr; --nr, vaddr += PGSZ)
        x86_64_unmap(PML4I(vaddr), PDPTI(vaddr), PDI(vaddr), PTI(vaddr));
}

int x86_64_map_i(uintptr_t vaddr, uintptr_t paddr, size_t sz, int flags) {
    int err = 0;
    uintptr_t vr = vaddr;
    size_t nr = NPAGE(sz);

    for (; nr; --nr, paddr += PGSZ, vaddr += PGSZ) {
        if ((err = x86_64_map(paddr, PML4I(vaddr),
            PDPTI(vaddr), PDI(vaddr), PTI(vaddr), flags)))
            goto error;
    }

    return 0;
error:
    nr = NPAGE(sz) - nr;
    x86_64_unmap_n(vr, nr * PGSZ);
    return err;
}

int x86_64_mprotect(uintptr_t vaddr, size_t sz, int flags) {
    int     err     = 0;
    int     mask    = 0;
    pte_t   *pte    = NULL;
    size_t  nr      = NPAGE(sz);
    vaddr   = PGROUND(vaddr);
    flags   = extract_vmflags(flags);

    if (_ispresent(flags) == 0)
        return -EINVAL;

    mask |= _isuser_page(flags)  == 0 ? PTE_U : 0;
    mask |= _isreadable(flags)   == 0 ? PTE_R : 0;
    mask |= _iswritable(flags)   == 0 ? PTE_W : 0;
    mask |= _isexecutable(flags) == 0 ? PTE_X : 0;
    mask = extract_vmflags(mask);

    while (nr--) {
        if ((err = x86_64_getmapping(vaddr, &pte)))
            return err;

        /**
         * @brief Mask out the page permissions we dont want
         */
        pte->raw &= ~mask; // Smart huh? ;)
        send_tlb_shootdown(rdcr3(), vaddr);
        vaddr += PGSZ;
    }

    return 0;
}

int x86_64_map_n(uintptr_t vaddr, size_t sz, int flags) {
    int         err = 0;
    uintptr_t   paddr   = 0;
    uintptr_t   vr  = vaddr;
    size_t      nr  = NPAGE(sz);
    gfp_mask_t  gfp_mask = GFP_NORMAL | (_iszero(flags) ? GFP_ZERO : 0);

    for (; nr; --nr, vaddr += PGSZ) {
        if ((paddr = pmman.get_page(gfp_mask)) == 0) {
            err = -ENOMEM;
            goto error;
        }
        if ((err = x86_64_map(paddr, PML4I(vaddr),
            PDPTI(vaddr), PDI(vaddr), PTI(vaddr), flags | PTE_ALLOC_PAGE)))
            goto error;
    }

    return 0;
error:
    nr = NPAGE(sz) - nr;
    x86_64_unmap_n(vr, nr * PGSZ);
    return err;
}

int x86_64_mount(uintptr_t paddr, void **pvp) {
    int         err = 0;
    uintptr_t   vaddr = 0;

    if (paddr == 0 || pvp == NULL)
        return -EINVAL;

    if ((vaddr = vmman.alloc(PGSZ)) == 0)
        return -ENOMEM;
    
    if ((err = x86_64_map_i(vaddr, paddr, PGSZ, PTE_KRW)))
        goto error;

    *pvp = (void *)vaddr;
    return 0;
error:
    if (vaddr)
        vmman.free(vaddr);
    return err;
}

void x86_64_unmount(uintptr_t vaddr) {
    x86_64_unmap_n(vaddr, PGSZ);
}

void x86_64_unmap_full(void) {
    size_t i4 = 0, i3 = 0, i2 = 0, i1 = 0;
    for (i4 = 0; i4 < PML4I(USTACK); ++i4) {
        if (!pte_ispresent(PML4E(i4)))
            continue;
        for (i3 = 0; i3 < NPTE; ++i3) {
            if (!pte_ispresent(PDPTE(i4, i3)))
                continue;
            for (i2 = 0; i2 < NPTE; ++i2) {
                if (!pte_ispresent(PDTE(i4, i3, i2)))
                    continue;
                for (i1 = 0; i1 < NPTE; ++i1) {
                    if (!pte_ispresent(PTE(i4, i3, i2, i1)))
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
    for (size_t i = PML4I(USTACK); i < NPTE; ++i)
        dstv[i] = PML4[i];
    
    dstv[510].raw = dstp | PTE_KRW | PTE_PCDWT;
    /// TODO: unlock higer vmmap and kernel pDBr.

    x86_64_unmount((uintptr_t)dstv);
    return 0;
}

int x86_64_lazycpy(uintptr_t dst, uintptr_t src) {
    int         err     = 0;
    uintptr_t   oldpdbr = 0;
    size_t      i4      = 0, i3 = 0, i2 = 0, i1 = 0;
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
        if (!pte_ispresent(&pml4[i4]))
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
            if (!pte_ispresent(&pdpt[i3]))
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
                if (!pte_ispresent(&pdt[i2]))
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
                    if (!pte_ispresent(&pt[i1]))
                        continue;

                    // Enforce Copy-On-Write by marking page ready-only.
                    if (pte_iswritable(&pt[i1])) {
                        pt[i1].w = 0;
                        send_tlb_shootdown(rdcr3(), __viraddr(i4, i3, i2, i1));
                    }
                    
                    // Do page copy.
                    PTE(i4, i3, i2, i1)->raw = pt[i1].raw;

                    // No need of incrementing MMIO address-space pages.                    
                    if (ismmio_addr(PGROUND(pt[i1].raw)))
                        continue;

                    // increase the page count on this page.
                    __page_incr(PGROUND(pt[i1].raw));
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

int x86_64_memcpypp(uintptr_t pdst, uintptr_t psrc, size_t size) {
    int         err     = 0;
    size_t      len     = 0;
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

int x86_64_memcpyvp(uintptr_t paddr, uintptr_t vaddr, size_t size) {
    int         err     = 0;
    size_t      len     = 0;
    uintptr_t   vdst    = 0;
    
    for (; size; size -= len, paddr += len, vaddr += len) {
        if ((err = x86_64_mount(PGROUND(paddr), (void **)&vdst)))
            return err;

        len = MIN(PAGESZ - PGOFF(paddr), size);
        memcpy((void *)(vdst + PGOFF(paddr)), (void *)vaddr, len);
        x86_64_unmount(vdst);
    }

    return 0;
}

int x86_64_memcpypv(uintptr_t vaddr, uintptr_t paddr, size_t size) {
    int         err     = 0;
    size_t      len     = 0;
    uintptr_t   vsrc    = 0;
    
    for (; size; size -= len, paddr += len, vaddr += len) {
        if ((err = x86_64_mount(PGROUND(paddr), (void **)&vsrc)))
            return err;

        len = MIN(PAGESZ - PGOFF(paddr), size);
        memcpy((void *)vaddr, (void *)(vsrc + PGOFF(paddr)), len);
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

    if (!pte_ispresent(PML4E(i4)))
        return -ENOENT;
    
    if (!pte_ispresent(PDPTE(i4, i3)))
        return -ENOENT;
    
    if (!pte_ispresent(PDTE(i4, i3, i2)))
        return -ENOENT;
    
    if (!pte_ispresent(PTE(i4, i3, i2, i1)))
        return -ENOENT;
    
    if (pte) *pte = PTE(i4, i3, i2, i1);
    return 0;
}

int x86_64_pml4alloc(uintptr_t *ref) {
    int         err     = 0;
    uintptr_t   pml4    = 0;

    if (ref == NULL)
        return -EINVAL;
    
    if ((pml4 = pmman.get_page(GFP_NORMAL | GFP_ZERO)) == 0)
        return -ENOMEM;
    
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