#include <arch/x86_64/pml4.h>
#include <arch/x86_64/system.h>
#include <bits/errno.h>
#include <lib/printk.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <lib/string.h>
#include <lib/types.h>
#include <mm/kalloc.h>
#include <mm/mm_zone.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <sys/system.h>
#include <mm/kalloc.h>

spinlock_t kmap_lk = SPINLOCK_INIT();
pagemap_t kernel_map = {
    .flags = 0,
    .pdbr = (void *)_PML4_,
    .lock = SPINLOCK_INIT(),
};

pte_t *x86_64_get_mapping(pte_t *pml4, uintptr_t v) {
    pte_t *pt = NULL;
    pte_t *pdt = NULL;
    pte_t *pdpt = NULL;

    if (!pml4 || !v)
        return NULL;

    if (iskernel_addr(v))
        kmap_assert_locked();

    // printk("pml4: %p\n", pml4);

    if (!pml4 || !pml4[PML4I(v)].p)
        return NULL;

    pdpt = (pte_t *)VMA2HI(PGROUND(pml4[PML4I(v)].raw));
    // printk("pdpt: %p\n", pdpt);

    if (!pdpt || !pdpt[PDPTI(v)].p)
        return NULL;

    pdt = (pte_t *)VMA2HI(PGROUND(pdpt[PDPTI(v)].raw));

    if (!pdt || !pdt[PDI(v)].p)
        return NULL;
    
    if (pdt[PDI(v)].ps)
        return &pdt[PDI(v)];

    pt = (pte_t *)VMA2HI(PGROUND(pdt[PDI(v)].raw));

    if (!pt || !pt[PTI(v)].p)
        return NULL;

    return &pt[PTI(v)];
}

static inline void x86_64_clrtable(pte_t *t) {
    for (int i = 0; i < NPTE; ++i)
        t[i].raw = 0;
}

int x86_64_map_r(uintptr_t frame, int i4, int i3, int i2, int i1, int flags) {
    int err = -ENOMEM;
    uintptr_t lv3 = 0, lv2 = 0, lv1 = 0;

    if (iL_INV(i4) || iL_INV(i3) || iL_INV(i2) || iL_INV(i1))
        return -EINVAL;
    
    if (PML4E(i4)->p == 0) {
        if (isPS(flags))
            return  -ENOTSUP;
        
        if ((lv3 = pmman.alloc()) == 0)
            goto error;
        
        PML4E(i4)->raw = lv3 | PGOFF(flags | VM_KRW | VM_PWT);
        invlpg((uintptr_t)PDPTE(i4, 0));
        x86_64_clrtable(PDPTE(i4, 0));
    }

    if (PDPTE(i4, i3)->p == 0) {
        if (isPS(flags)) {
            err = -ENOTSUP;
            goto error;
        }

        if ((lv2 = pmman.alloc()) == 0) {
            err = -ENOMEM;
            goto error;
        }

        PDPTE(i4, i3)->raw = lv2 | PGOFF(flags | VM_KRW | VM_KRW);
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        x86_64_clrtable(PDTE(i4, i3, 0));
    }

    if (PDTE(i4, i3, i2)->p == 0) {
        if (isPS(flags)) {
            err = -ENOTSUP;
            goto error;
        }

        if ((lv1 = pmman.alloc()) == 0) {
            err = -ENOMEM;
            goto error;
        }

        PDTE(i4, i3, i2)->raw = lv1 | PGOFF(flags | VM_KRW | VM_KRW);
        invlpg((uintptr_t)PTE(i4, i3, i2, 0));
        x86_64_clrtable(PTE(i4, i3, i2, 0));
    }

    PTE(i4, i3, i2, i1)->raw = PGROUND(frame) | PGOFF(flags);
    invlpg(__viraddr(i4, i3, i2, i1));
    return 0;
error:
    if (lv2) {
        PDPTE(i4, i3)->raw = 0;
        invlpg((uintptr_t)PDTE(i4, i3, 0));
        pmman.free(lv2);
    }

    if (lv3) {
        PML4E(i4)->raw = 0;
        invlpg((uintptr_t)PDPTE(i4, 0));
        pmman.free(lv3);
    }
    return err;
}

int x86_64_map_to(pte_t *pml4, uintptr_t v, uintptr_t p, uint32_t flags) {
    int err = 0;
    pte_t *pt = NULL;
    pte_t *pdt = NULL;
    uintptr_t page = 0;
    pte_t *pdpt = NULL;

    if (!pml4 || !v)
        return -EINVAL;

    if (iskernel_addr(v))
        kmap_assert_locked();

    // printk("going on to map\n");

    if (!pml4[PML4I(v)].p) {
        // printk("pml4e: not aval\n");
        err = -ENOMEM;
        if (!(page = pmman.alloc()))
            goto error;

        // printk("allocated pml4e at: %p\n", p);
        memset((void *)VMA2HI(page), 0, PGSZ);
        pml4[PML4I(v)].raw = page | ALLOC_PAGE | ((flags & VM_U) ? VM_URW : VM_KRW);
    }

    pdpt = (pte_t *)VMA2HI(PGROUND(pml4[PML4I(v)].raw));

    if (!pdpt[PDPTI(v)].p) {
        // printk("pdpte: not aval\n");
        err = -ENOMEM;
        if (!(page = pmman.alloc()))
            goto error;

        // printk("allocated pdpte at: %p\n", page);
        memset((void *)VMA2HI(page), 0, PGSZ);
        pdpt[PDPTI(v)].raw = page | ALLOC_PAGE | ((flags & VM_U) ? VM_URW : VM_KRW);
    }

    pdt = (pte_t *)VMA2HI(PGROUND(pdpt[PDPTI(v)].raw));

    if (is2mb_page(flags)) {
        assert_msg(!PG2MOFF(p), "%s:%d: error: page: %p, not 2MiB aligned\n", __FILE__, __LINE__, p);
        
        //printk("mapping a 2MiB page\n");
        if (pdt[PDI(v)].p) {
            if (!(flags & REMAPPG))
                return -EALREADY;
        }

        // printk("Mapping; %p -> %p\n", v, p);
        pdt[PDI(v)].raw = p | AND(flags, PGMASK);

        //printk("%s:%d WARNING: need to send tlb shootdown\n", __FILE__, __LINE__);
        invlpg(v);

        return 0;
    }

    // printk("Not a 2Mib page\n");

    if (!pdt[PDI(v)].p) {
        // printk("pde: not aval\n");
        err = -ENOMEM;
        if (!(page = pmman.alloc()))
            goto error;

        // printk("allocated pde at: %p\n", page);
        memset((void *)VMA2HI(page), 0, PGSZ);
        pdt[PDI(v)].raw = page | ALLOC_PAGE | ((flags & VM_U) ? VM_URW : VM_KRW);
    }

    if (pdt[PDI(v)].ps) {
        if (!(flags & REMAPPG))
            return -EALREADY;

        // printk("pde: not aval\n");
        err = -ENOMEM;
        if (!(page = pmman.alloc()))
            goto error;

        // printk("allocated pde at: %p\n", page);
        memset((void *)VMA2HI(page), 0, PGSZ);
        pdt[PDI(v)].raw = page | ALLOC_PAGE | ((flags & VM_U) ? VM_URW : VM_KRW);
    }

    pt = (pte_t *)VMA2HI(PGROUND(pdt[PDI(v)].raw));

    assert_msg(!PGOFF(p), "%s:%d: error: page: %p, not 4KiB aligned\n", __FILE__, __LINE__, p);
    
    if (pt[PTI(v)].p) {
        if (!(flags & REMAPPG))
            return -EALREADY;

        if (pt[PTI(v)].alloc)
        {
            page = GETPHYS(&pt[PTI(v)]);
            pmman.free(page);
        }
    }

    pt[PTI(v)].raw = p | AND(flags, PGMASK);
    invlpg(v);

    //printk("%s:%d WARNING: need to send tlb shootdown\n", __FILE__, __LINE__);
    //printk("VADDR: %p, PADDR: %p\n", v, pt[PTI(v)].raw);

    return 0;
error:
    return err;
}

int x86_64_map_to_n(pte_t *pml4, uintptr_t v, uintptr_t p, size_t sz, uint32_t flags) {
    int err = 0;
    size_t np = is2mb_page(flags) ? N2MPAGE(sz) : NPAGE(sz);

    if (iskernel_addr(v))
        kmap_assert_locked();

    while (np--) {
        if ((err = x86_64_map_to(pml4, v, p, flags)))
            goto error;
        v += is2mb_page(flags) ? PGSZ2M : PGSZ;
        p += is2mb_page(flags) ? PGSZ2M : PGSZ;
    }

    return 0;
error:
    return err;
}

int x86_64_map(pte_t *pml4, uintptr_t v, size_t sz, uint32_t flags) {
    int         err = 0;
    uintptr_t   p = 0;
    flags       &= ~VM_PS;
    size_t      np = NPAGE(sz);

    if (iskernel_addr(v))
        kmap_assert_locked();

    while (np--) {
        err = -ENOMEM;
        if (!(p = pmman.alloc()))
            goto error;
        if ((err = x86_64_map_to(pml4, v, p, flags | ALLOC_PAGE)))
            goto error;
        v += PGSZ;
    }

    return 0;
error:
    return err;
}

int x86_64_unmap_table_entry(pte_t *pml4, int level, int dp, int pdpti, int pdi, int pti) {
    pte_t *pt = NULL;
    pte_t *pdt = NULL;
    uintptr_t page = 0;
    pte_t *pdpt = NULL;

    if (!pml4)
        return -EINVAL;

    if (dp >= PML4I(VMA_BASE))
        kmap_assert_locked();

    if (!pml4[dp].p)
        return -ENOENT;

    if (level == LVL_PML4E) {
        page = GETPHYS(&pml4[dp]);
        if (page && pml4[dp].alloc)
            pmman.free(page);
        
        pml4[dp].raw = 0;

        printk("%s:%d: WARNING: send TLB SHOOTDOWN\n", __FILE__, __LINE__);
        invlpg(__viraddr(dp, pdpti, pdi, pti));
        return 0;
    }

    pdpt = (pte_t *)VMA2HI(PGROUND(pml4[dp].raw));

    if (!pdpt || !pdpt[pdpti].p)
        return -ENOENT;

    if (level == LVL_PDPTE) {
        // printk("LVL_PDPTE\n");
        page = GETPHYS(&pdpt[pdpti]);
        if (page && pdpt[pdpti].alloc)
            pmman.free(page);

        pdpt[pdpti].raw = 0;

        invlpg(__viraddr(dp, pdpti, pdi, pti));
        printk("%s:%d: WARNING: send TLB SHOOTDOWN\n", __FILE__, __LINE__);
        return 0;
    }

    pdt = (pte_t *)VMA2HI(PGROUND(pdpt[pdpti].raw));

    if (!pdt || !pdt[pdi].p)
        return -ENOENT;

    if (level == LVL_PDTE) {
        if (!pdt[pdi].ps) {
            page = GETPHYS(&pdt[pdi]);
            if (page && pdt[pdi].alloc)
                pmman.free(page);
        }

        pdt[pdi].raw = 0;
        invlpg(__viraddr(dp, pdpti, pdi, pti));
        printk("%s:%d: WARNING: send TLB SHOOTDOWN\n", __FILE__, __LINE__);
        return 0;
    }

    if (pdt[pdi].ps)
        return -EINVAL;

    pt = (pte_t *)VMA2HI(PGROUND(pdt[pdi].raw));

    if (!pt || !pt[pti].p)
        return -ENOENT;

    if (level == LVL_PTE) {
        page = GETPHYS(&pt[pti]);
        if (page && pt[pti].alloc)
            pmman.free(page);

        pt[pti].raw = 0;
        printk("%s:%d: WARNING: send TLB SHOOTDOWN\n", __FILE__, __LINE__);
        invlpg(__viraddr(dp, pdpti, pdi, pti));
        return 0;
    }

    return -EINVAL;
}

int x86_64_unmap(pte_t *pml4, uintptr_t v) {
    pte_t *pt = NULL;
    pte_t *pdt = NULL;
    uintptr_t page = 0;
    pte_t *pdpt = NULL;

    if (!pml4)
        return -EINVAL;

    if (iskernel_addr(v))
        kmap_assert_locked();

    if (!pml4[PML4I(v)].p)
        return -EINVAL;

    pdpt = (pte_t *)VMA2HI(PGROUND(pml4[PML4I(v)].raw));

    if (!pdpt || !pdpt[PDPTI(v)].p)
        return -EINVAL;

    pdt = (pte_t *)VMA2HI(PGROUND(pdpt[PDPTI(v)].raw));

    if (!pdt || !pdt[PDI(v)].p)
        return -ENOENT;

    if (is2mb_page(pdt[PDI(v)].raw)) {
        pdt[PDI(v)].raw = 0;
        // printk("unmap 2Mb\n");
        // printk("%s:%d WARNING: need to send tlb shootdown\n", __FILE__, __LINE__);
        invlpg(v);
        return 0;
    }

    pt = (pte_t *)VMA2HI(PGROUND(pdt[PDI(v)].raw));

    if (!pt || !pt[PTI(v)].p)
        return -ENOENT;

    if (pt[PTI(v)].alloc) {
        page = GETPHYS(&pt[PTI(v)]);
        pmman.free(page);
    }

    pt[PTI(v)].raw = 0;
    // printk("umap 4Kb page\n");
    // printk("%s:%d WARNING: need to send tlb shootdown\n", __FILE__, __LINE__);
    invlpg(v);
    return 0;
}

int x86_64_unmap_n(pte_t *pml4, uintptr_t v, size_t sz, uint32_t flags) {
    int err = 0;
    size_t np = is2mb_page(flags) ? N2MPAGE(sz) : NPAGE(sz);

    if (iskernel_addr(v))
        kmap_assert_locked();

    while (np--) {
        if ((err = x86_64_unmap(pml4, v)))
            return err;
        v += is2mb_page(flags) ? PGSZ2M : PGSZ;
    }
    return 0;
}

int x86_64_swtchvm(pte_t *pml4, pte_t **oldpml4) {
    if (pml4 == NULL)
        return -EINVAL;
    if (oldpml4)
        *oldpml4 = (pte_t *)rdcr3();
    wrcr3((uintptr_t)pml4);
    return 0;
}

void x86_64_tlb_flush(void) {
    wrcr3(rdcr3());
}

int x86_64_unmappdbr(pte_t *pml4) {
    pte_t *pdpt = NULL;
    pte_t *pdt = NULL;
    pte_t *pt = NULL;

    if (pml4 == NULL)
        return -EINVAL;
    
    for (int i = 0; i < NPTE; ++i) {
        pdpt = (pte_t *)VMA2HI(GETPHYS(&pml4[i]));
        for (int i = 0; i < NPTE; ++i) {
            pdt = (pte_t *)VMA2HI(GETPHYS(&pdpt[i]));
            for (int i = 0; i < NPTE; ++i) {
                pt = (pte_t *)VMA2HI(GETPHYS(&pdt[i]));
                for (int i = 0; i < NPTE; ++i) {
                    pdpt = (pte_t *)VMA2HI(GETPHYS(&pt[i]));
                }
            }
        }
    }

    return -ENOSYS;
}

int x86_64_mappdpt(int d, int flags) {
    __unused int err = 0;
    uintptr_t addr = 0;

    if (d < 0 || d > NPTE)
        return -EINVAL;

    if ((flags & VM_U))
        flags &= VM_W;
    
    PML4E(d)->raw = PGROUND(addr) | PGOFF(VM_PWT | flags);

    return 0;
}

int x86_64_mapt(pte_t *pml4 __unused, size_t t __unused) {
    return -ENOSYS;
}

int x86_64_unmapt(pte_t *pml4 __unused, size_t t __unused) {
    __unused int level = LVL_PDPTE;
    
    return -ENOSYS;
}

int x86_64_mapv_n(pte_t *pml4, uintptr_t v, size_t sz, int flags) {
    return x86_64_map(pml4, v, sz, flags);
}

int x86_64_mappv(pte_t *pml4, uintptr_t p, uintptr_t v, size_t sz, int flags) {
    return x86_64_map_to_n(pml4, v, p, sz, flags);
}

int x86_64_swtchkvm(void) {
    int err = 0;
    pagemap_binary_lock(&kernel_map);
    err = x86_64_swtchvm((void *)kernel_map.pdbr, NULL);
    pagemap_binary_unlock(&kernel_map);
    return err;
}

uintptr_t x86_64_getpdbr(void) {
    return rdcr3();
}

int x86_64_kvmcpy(pte_t *dst) {
    if (dst == NULL)
        return -EINVAL;
    pagemap_binary_lock(&kernel_map);
    for (int i = PML4I(VMA_BASE); i < 512; ++i)
        dst[i] = ((pte_t *)kernel_map.pdbr)[i];
    pagemap_binary_lock(&kernel_map);
    return 0;
}

int x86_64_lazycpy(pte_t *dst __unused, pagemap_t *src __unused) {
    return -ENOSYS;
}