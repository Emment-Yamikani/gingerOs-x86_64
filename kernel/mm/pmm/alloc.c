#include <arch/paging.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <mm/zone.h>
#include <sys/thread.h>

static int zero_fill_page(zone_t *zone, page_t *page, int whence) {
    int         err     = 0;
    void        *vaddr  = NULL;
    uintptr_t   paddr   = zone->start + ((page - zone->pages) * PGSZ);

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
        vaddr = (void *)VMA2HI(paddr);
        memset(vaddr, 0, PGSZ);
    }

    return 0;
}

static inline int validate_page_alloc_input(gfp_t gfp, usize order, page_t **pp) {
    if (pp == NULL)
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
 * @param pp Output pointer to the first page in the allocated range.
 * @return 0 on success, negative error code on failure.
 */
int page_alloc_n(gfp_t gfp, usize order, page_t **pp) {
    int         err     = 0;
    int         whence  = 0;
    usize       index   = 0;
    page_t      *page   = NULL;
    zone_t      *zone   = NULL;
    usize       npage   = BS(order);

    if ((err = validate_page_alloc_input(gfp, order, pp)))
        return err;

    whence = gfp_to_zone_index(gfp);
    if (whence < 0)
        return -EINVAL;

    loop() {
        if ((err = getzone_byindex(whence, &zone)))
            return err;
        
        if ((err = bitmap_alloc_range(&zone->bitmap, npage, &index))) {
            zone_unlock(zone);
            return err;
        }

        page = &zone->pages[index];
        // printk("index: %d: %p\n", index, page_addr(page, zone));

        for (; npage; --npage, ++page) {
            assert_msg(page_addr(page, zone) != zones[ZONEi_NORM].start,
                "%s(): %s:%d: Page belongs to kernel, page: %p\n",
                __func__, __FILE__, __LINE__, page_addr(page, zone)
            );

            assert_msg(!atomic_read(&page->refcnt),
                "%s(): %s:%d: Page[%p]: page->refcnt: %ld\nZONE: %p-%p, size: %d\n",
               __func__,  __FILE__, __LINE__, page_addr(page, zone), page->refcnt,
               zone->start, zone_end(zone), zone->size
            );

            atomic_inc(&page->refcnt);

            // does caller want a zero-filled page?
            if (gfp & GFP_ZERO) {
                if ((err = zero_fill_page(zone, page, whence))) {
                    zone_unlock(zone);
                    return err;
                }
            }
        }

        zone->upages += BS(order);
        *pp = &zone->pages[index];
        zone_unlock(zone);
        return 0;
    }
}

static int do_page_alloc(gfp_t gfp, usize order, page_t **ppage, void **pp) {
    int     err     = 0;
    page_t  *page   = NULL;

    if (pp == NULL && ppage == NULL)
        return -EINVAL;

    if ((err = page_alloc_n(gfp, order, &page)))
        return err;

    if (pp) {
        if ((err = page_get_address(page, pp))) {
            page_free_n(page, order);
            return err;
        }
    }

    if (ppage) *ppage = page;

    return 0;
}

int page_alloc(gfp_t gfp, page_t **pp) {
    return do_page_alloc(gfp, 0, pp, NULL);
}

int __page_alloc_n(gfp_t gfp, usize order, void **pp) {
    return do_page_alloc(gfp, order, NULL, pp);
}

int __page_alloc(gfp_t gfp, void **pp) {
    return do_page_alloc(gfp, 0, NULL, pp);
}