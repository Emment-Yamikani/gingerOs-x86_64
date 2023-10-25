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

extern pte_t __pml4[512] __aligned(0x1000);

spinlock_t kmap_lk = SPINLOCK_INIT();
pagemap_t kernel_map = {
    .flags = 0,
    .pml4 = __pml4,
    .lock = SPINLOCK_INIT(),
};

pte_t *get_mapping(pagemap_t *map, uintptr_t v)
{
    pte_t *pt = NULL;
    pte_t *pdt = NULL;
    pte_t *pdpt = NULL;
    pte_t *pml4 = NULL;

    if (!map || !v)
        return NULL;

    pagemap_assert_locked(map);

    if (iskernel_addr(v))
        kmap_assert_locked();

    pml4 = map->pml4;
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

int map_page_to(pagemap_t *map, uintptr_t v, uintptr_t p, uint32_t flags)
{
    int err = 0;
    pte_t *pt = NULL;
    pte_t *pdt = NULL;
    uintptr_t page = 0;
    pte_t *pdpt = NULL;
    pte_t *pml4 = NULL;

    if (!map || !v)
        return -EINVAL;

    pagemap_assert_locked(map);

    if (iskernel_addr(v))
        kmap_assert_locked();

    pml4 = map->pml4;

    if (!pml4)
        return -EINVAL;

    // printk("going on to map\n");

    if (!pml4[PML4I(v)].p)
    {
        // printk("pml4e: not aval\n");
        err = -ENOMEM;
        if (!(page = pmman.alloc()))
            goto error;

        // printk("allocated pml4e at: %p\n", p);
        memset((void *)VMA2HI(page), 0, PGSZ);
        pml4[PML4I(v)].raw = page | ALLOC_PAGE | ((flags & VM_U) ? VM_URW : VM_KRW);
    }

    pdpt = (pte_t *)VMA2HI(PGROUND(pml4[PML4I(v)].raw));

    if (!pdpt[PDPTI(v)].p)
    {
        // printk("pdpte: not aval\n");
        err = -ENOMEM;
        if (!(page = pmman.alloc()))
            goto error;

        // printk("allocated pdpte at: %p\n", page);
        memset((void *)VMA2HI(page), 0, PGSZ);
        pdpt[PDPTI(v)].raw = page | ALLOC_PAGE | ((flags & VM_U) ? VM_URW : VM_KRW);
    }

    pdt = (pte_t *)VMA2HI(PGROUND(pdpt[PDPTI(v)].raw));

    if (is2mb_page(flags))
    {
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

    if (!pdt[PDI(v)].p)
    {
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

int map_page_to_n(pagemap_t *map, uintptr_t v, uintptr_t p, size_t sz, uint32_t flags)
{
    int err = 0;
    size_t np = is2mb_page(flags) ? N2MPAGE(sz) : NPAGE(sz);

    pagemap_assert_locked(map);

    if (iskernel_addr(v))
        kmap_assert_locked();

    while (np--)
    {
        if ((err = map_page_to(map, v, p, flags)))
            goto error;
        v += is2mb_page(flags) ? PGSZ2M : PGSZ;
        p += is2mb_page(flags) ? PGSZ2M : PGSZ;
    }

    return 0;
error:
    return err;
}

int map_page(pagemap_t *map, uintptr_t v, size_t sz, uint32_t flags)
{
    int err = 0;
    uintptr_t p = 0;
    flags &= ~VM_PS;
    size_t np = NPAGE(sz);

    pagemap_assert_locked(map);

    if (iskernel_addr(v))
        kmap_assert_locked();

    while (np--)
    {
        err = -ENOMEM;
        if (!(p = pmman.alloc()))
            goto error;
        if ((err = map_page_to(map, v, p, flags | ALLOC_PAGE)))
            goto error;
        v += PGSZ;
    }

    return 0;
error:
    return err;
}

int unmap_table_entry(pagemap_t *map, int level, int pml4i, int pdpti, int pdi, int pti) {
    pte_t *pt = NULL;
    pte_t *pdt = NULL;
    uintptr_t page = 0;
    pte_t *pdpt = NULL;
    pte_t *pml4 = NULL;

    if (!map)
        return -EINVAL;

    pagemap_assert_locked(map);

    if (pml4i >= PML4I(VMA_BASE))
        kmap_assert_locked();

    pml4 = map->pml4;

    if (!pml4 || !pml4[pml4i].p)
        return -ENOENT;

    if (level == LVL_PML4E) {
        page = GETPHYS(&pml4[pml4i]);
        if (page && pml4[pml4i].alloc)
            pmman.free(page);
        
        pml4[pml4i].raw = 0;

        printk("%s:%d: WARNING: send TLB SHOOTDOWN\n", __FILE__, __LINE__);
        invlpg(__viraddr(pml4i, pdpti, pdi, pti));
        return 0;
    }

    pdpt = (pte_t *)VMA2HI(PGROUND(pml4[pml4i].raw));

    if (!pdpt || !pdpt[pdpti].p)
        return -ENOENT;

    if (level == LVL_PDPTE)
    {
        // printk("LVL_PDPTE\n");
        page = GETPHYS(&pdpt[pdpti]);
        if (page && pdpt[pdpti].alloc)
            pmman.free(page);

        pdpt[pdpti].raw = 0;

        invlpg(__viraddr(pml4i, pdpti, pdi, pti));
        printk("%s:%d: WARNING: send TLB SHOOTDOWN\n", __FILE__, __LINE__);
        return 0;
    }

    pdt = (pte_t *)VMA2HI(PGROUND(pdpt[pdpti].raw));

    if (!pdt || !pdt[pdi].p)
        return -ENOENT;

    if (level == LVL_PDTE)
    {
        if (!pdt[pdi].ps) {
            page = GETPHYS(&pdt[pdi]);
            if (page && pdt[pdi].alloc)
                pmman.free(page);
        }

        pdt[pdi].raw = 0;
        invlpg(__viraddr(pml4i, pdpti, pdi, pti));
        printk("%s:%d: WARNING: send TLB SHOOTDOWN\n", __FILE__, __LINE__);
        return 0;
    }

    if (pdt[pdi].ps)
        return -EINVAL;

    pt = (pte_t *)VMA2HI(PGROUND(pdt[pdi].raw));

    if (!pt || !pt[pti].p)
        return -ENOENT;

    if (level == LVL_PTE)
    {
        page = GETPHYS(&pt[pti]);
        if (page && pt[pti].alloc)
            pmman.free(page);

        pt[pti].raw = 0;
        printk("%s:%d: WARNING: send TLB SHOOTDOWN\n", __FILE__, __LINE__);
        invlpg(__viraddr(pml4i, pdpti, pdi, pti));
        return 0;
    }

    return -EINVAL;
}

int unmap_page(pagemap_t *map, uintptr_t v)
{
    pte_t *pt = NULL;
    pte_t *pdt = NULL;
    uintptr_t page = 0;
    pte_t *pdpt = NULL;
    pte_t *pml4 = NULL;

    if (!map)
        return -EINVAL;

    pagemap_assert_locked(map);

    if (iskernel_addr(v))
        kmap_assert_locked();

    pml4 = map->pml4;

    if (!pml4 || !pml4[PML4I(v)].p)
        return -EINVAL;

    if (v >= VMA2HI(0) && (v <= VMA2HI(2 * GiB)))
        return -EINVAL;

    pdpt = (pte_t *)VMA2HI(PGROUND(pml4[PML4I(v)].raw));

    if (!pdpt || !pdpt[PDPTI(v)].p)
        return -EINVAL;

    pdt = (pte_t *)VMA2HI(PGROUND(pdpt[PDPTI(v)].raw));

    if (!pdt || !pdt[PDI(v)].p)
        return -ENOENT;

    if (is2mb_page(pdt[PDI(v)].raw))
    {
        pdt[PDI(v)].raw = 0;
        // printk("unmap 2Mb\n");
        //printk("%s:%d WARNING: need to send tlb shootdown\n", __FILE__, __LINE__);
        invlpg(v);
        return 0;
    }

    pt = (pte_t *)VMA2HI(PGROUND(pdt[PDI(v)].raw));

    if (!pt || !pt[PTI(v)].p)
        return -ENOENT;

    if (pt[PTI(v)].alloc)
    {
        page = GETPHYS(&pt[PTI(v)]);
        pmman.free(page);
    }

    pt[PTI(v)].raw = 0;
    // printk("umap 4Kb page\n");
    //printk("%s:%d WARNING: need to send tlb shootdown\n", __FILE__, __LINE__);
    invlpg(v);
    return 0;
}

int unmap_page_n(pagemap_t *map, uintptr_t v, size_t sz, uint32_t flags)
{
    int err = 0;
    size_t np = is2mb_page(flags) ? N2MPAGE(sz) : NPAGE(sz);

    pagemap_assert_locked(map);

    if (iskernel_addr(v))
        kmap_assert_locked();

    while (np--)
    {
        if ((err = unmap_page(map, v)))
            return err;
        v += is2mb_page(flags) ? PGSZ2M : PGSZ;
    }
    return 0;
}

void pagemap_resolve(void) {
    pagemap_t *map = NULL;
    pagemap_alloc(&map);
    pagemap_binary_lock(map);
    pagemap_switch(map);
    memcpy(&kernel_map, map, sizeof *map);
    pagemap_binary_unlock(map);
}

void pagemap_clean(pagemap_t *map) {
    if (!map)
        return;
    
    pagemap_assert_locked(map);
    for (uintptr_t v = 0; v < USTACK; v += PGSZ)
        unmap_page(map, v);
}

void pagemap_free(pagemap_t *map) {
    if (!map)
        return;
    
    if (map->pml4) {

        for (int i = 0; i < PML4I(VMA_BASE); ++i) {

        }
        mapped_free((uintptr_t)map->pml4, PGSZ);
    }
    kfree(map);

}

int pagemap_alloc(pagemap_t **ppagemap) {
    int err = -ENOMEM;
    pte_t *pml4 = NULL;
    pagemap_t *map = NULL;

    if (!(map = kmalloc(sizeof *map)))
        goto error;

    if (!(pml4 = (pte_t *)mapped_alloc(PGSZ)))
        goto error;

    memset(pml4, 0, PGSZ);
    memset(map, 0, sizeof *map);

    pagemap_binary_lock(&kernel_map);

    for (int i = PML4I(VMA_BASE); i < 512; ++i)
        pml4[i] = kernel_map.pml4[i];

    pagemap_binary_unlock(&kernel_map);

    map->pml4 = pml4;
    map->flags = BS(0);
    map->lock = SPINLOCK_INIT();

    *ppagemap = map;
    return 0;
error:
    if (map) kfree(map);
    if (pml4) mapped_free((uintptr_t)pml4, PGSZ);
    return err;
}

int pagemap_switch(pagemap_t *map) {
    if (!map)
        return -EINVAL;
    pagemap_assert_locked(map);
    if (!GETPHYS(map->pml4))
        return -ENOENT;
    wrcr3(GETPHYS(map->pml4));
    return 0; 
}