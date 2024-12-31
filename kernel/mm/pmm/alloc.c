#include <arch/paging.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <mm/zone.h>
#include <sys/thread.h>


int page_alloc_n(gfp_t gfp, usize order, page_t **pp) {
    int         err     = 0;
    int         whence  = 0;
    uintptr_t   paddr   = 0;
    void        *vaddr  = 0;
    usize       index   = 0;
    page_t      *page   = NULL;
    zone_t      *zone   = NULL;
    usize       npage   = BS(order);

    if (pp == NULL)
        return -EINVAL;

    if ((order >= 64))
        return -ENOMEM;

    if (GFP_WHENCE(gfp) > __GFP_HIGHMEM)
        return -EINVAL;

    if (GFP_WHENCE(gfp) == __GFP_ANY)
        whence = ZONEi_NORM;
    else if (GFP_WHENCE(gfp) == __GFP_DMA)
        whence = ZONEi_DMA;
    else if (GFP_WHENCE(gfp) == __GFP_NORMAL)
        whence = ZONEi_NORM;
    else if (GFP_WHENCE(gfp) == __GFP_HOLE)
        whence = ZONEi_HOLE;
    else if (GFP_WHENCE(gfp) == __GFP_HIGHMEM)
        whence = ZONEi_HIGH;

    loop() {
        if ((err = getzone_byindex(whence, &zone)))
            return err;
        
        if ((err = bitmap_alloc_range(&zone->bitmap, npage, &index))) {
            zone_unlock(zone);
            return err;
        }

        page = &zone->pages[index];
        printk("index: %d: %p\n", index, page_addr(page, zone));

        for (; npage; --npage, ++page) {
            assert_msg(page_addr(page, zone) != zones[ZONEi_NORM].start,
                "%s@%s:%d: Page belongs to kernel, page: %p\n",
                __func__, __FILE__, __LINE__, page_addr(page, zone)
            );

            assert_msg(!atomic_read(&page->refcnt),
                "%s@%s:%d: Page refcnt != 0??: [%p]: page->refcnt: %ld\n",
               __func__,  __FILE__, __LINE__, page_addr(page, zone), page->refcnt
            );

            atomic_inc(&page->refcnt);

            // does caller want a zero-filled page?
            if (gfp & GFP_ZERO) {
                // get the physical address of this page.
                paddr = zone->start + ((page - zone->pages) * PGSZ);
                if ((whence == ZONEi_HOLE) || (whence == ZONEi_HIGH)) {
                    /// attempt a page frame mount.
                    /// spin in a loop for now unpon failure.
                    /// TODO: implement a more plausible approach than spinning.
                    while ((err = arch_mount(paddr, &vaddr))) {
                        panic("%s:%d: Failed to mount, err: %d\n", __FILE__, __LINE__, err);
                    }

                    // clear mounted page.
                    memset(vaddr, 0, PGSZ);
                    // unmount the page frame.
                    arch_unmount((uintptr_t)vaddr);
                } else {
                    /// addresses from 0->2GiB are indically mapped.
                    /// so just convert the paddr directly to vaddr.
                    vaddr = (void *)VMA2HI(paddr);
                    memset(vaddr, 0, PGSZ);
                }
            }
        }

        zone->upages += BS(order);
        *pp = &zone->pages[index];
        zone_unlock(zone);
        return 0;
    }
}

int page_alloc(gfp_t gfp, page_t **pp) {
    return page_alloc_n(gfp, 0, pp);
}

int __page_alloc_n(gfp_t gfp, usize order, void **pp) {
    int     err     = 0;
    page_t  *page   = NULL;

    if (pp == NULL)
        return -EINVAL;

    if ((err = page_alloc_n(gfp, order, &page)))
        return err;
    
    if ((err = page_get_address(page, pp))) {
        page_free_n(page, order);
        return -err;
    }

    return 0;
}

int __page_alloc(gfp_t gfp, void **pp) {
    return __page_alloc_n(gfp, 0, pp);
}