#include <arch/paging.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <sys/thread.h>
#include <mm/zone.h>

static int zero_fill_page(zone_t *zone, page_t *page, int whence) {
    int         err     = 0;
    void        *vaddr  = NULL;
    uintptr_t   paddr   = 0;

    zone_assert(zone);

    paddr   = zone->start + ((page - zone->pages) * PGSZ);

    if ((whence == ZONEi_HOLE) || (whence == ZONEi_HIGH)) {
        // Handle high memory or hole zone
        while ((err = arch_mount(paddr, &vaddr))) {
            panic("%s:%d: Failed to mount, err: %d\n", __FILE__, __LINE__, err);
        }
        memset(vaddr, 0, PGSZ);
        arch_unmount((uintptr_t)vaddr);
    } else {
        /// addresses from 0->2GiB are indically mapped.
        /// so just convert the paddr directly to vaddr.
        vaddr = (void *)V2HI(paddr);
        memset(vaddr, 0, PGSZ);
    }

    return 0;
}

static inline int validate_input(gfp_t gfp, usize order, page_t **ppage, void **ppaddr) {
    if (ppage == NULL && ppaddr == NULL)
        return -EINVAL;
    if (order >= MAX_PAGE_ORDER)
        return -ENOMEM;
    if (GFP_WHENCE(gfp) > __GFP_HIGHMEM)
        return -EINVAL;
    return 0;
}

static inline int gfp_to_zone_index(gfp_t gfp) {
    switch (GFP_WHENCE(gfp)) {
    case __GFP_ANY:    return ZONEi_NORM;
    case __GFP_DMA:    return ZONEi_DMA;
    case __GFP_NORMAL: return ZONEi_NORM;
    case __GFP_HOLE:   return ZONEi_HOLE;
    case __GFP_HIGHMEM: return ZONEi_HIGH;
    default: return -EINVAL;
    }
}

/**
 * Allocate a contiguous range of page frames.
 *
 * @param gfp GFP flags specifying allocation type.
 * @param order Number of contiguous pages as a power of 2.
 * @param ppage Output pointer to the first page in the allocated range.
 * @param ppaddr Output pointer to physical(addr in RAM) location of the range.
 * @return 0 on success, negative error code on failure.
 */
static int do_page_alloc_n(gfp_t gfp, usize order, page_t **ppage, void **ppaddr) {
    int         err     = 0;
    int         whence  = 0;
    usize       index   = 0;
    page_t      *page   = NULL;
    zone_t      *zone   = NULL;
    usize       npage   = BS(order);

    if ((err = validate_input(gfp, order, ppage, ppaddr)))
        return err;

    if ((whence = err = gfp_to_zone_index(gfp)) < 0)
        return err;

    loop() {
        if ((err = getzone_byindex(whence, &zone)))
            return err;

        if ((err = bitmap_alloc_range(&zone->bitmap, npage, &index))) {
            zone_unlock(zone);
            return err;
        }

        for (page = &zone->pages[index]; npage--; ++page) {
            assert(!OVERLAPS(page_addr(page, zone), PGSZ,
                bootinfo.kern_base, bootinfo.kern_size),
                "Page: [%p] Overlaps the kernel image.\n",
                page_addr(page, zone)
            );

            assert(!atomic_read(&page->refcnt),
                "Page: [%p] already has refcnt: %ld??\n",
                page_addr(page, zone), page->refcnt
            );

            atomic_inc(&page->refcnt);

            // does caller want a zero-filled page?
            if (gfp & GFP_ZERO) {
                if ((err = zero_fill_page(zone, page, whence))) {
                    // catch error.
                    assert(0, "Failed to zero-fill page[%p]. error: %d\n",
                        page_addr(page, zone), err
                    );
                    zone_unlock(zone);
                    return err;
                }
            }
        }

        zone->upages += BS(order);
        page    = &zone->pages[index];
        
        if (ppage)
            *ppage  = page;
        
        if (ppaddr)
            *ppaddr = (void *)page_addr(page, zone);
        
        zone_unlock(zone);
        return 0;
    }
}

int page_alloc_n(gfp_t gfp, usize order, page_t **pp) {
    return do_page_alloc_n(gfp, order, pp, NULL);
}

int page_alloc(gfp_t gfp, page_t **pp) {
    return do_page_alloc_n(gfp, 0, pp, NULL);
}

int __page_alloc_n(gfp_t gfp, usize order, void **pp) {
    return do_page_alloc_n(gfp, order, NULL, pp);
}

int __page_alloc(gfp_t gfp, void **pp) {
    return do_page_alloc_n(gfp, 0, NULL, pp);
}

int page_alloc_x(gfp_t gfp, usize order, page_t **ppage, void **paddr) {
    return do_page_alloc_n(gfp, order, ppage, paddr);
}