#include <bits/errno.h>
#include <core/misc.h>
#include <mm/page.h>
#include <mm/zone.h>
#include <mm/pmm.h>
#include <sync/atomic.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/sysproc.h>
#include <sys/system.h>
#include <sys/sched.h>

size_t mem_free(void);
size_t mem_used(void);
void mm_free(uintptr_t);
uintptr_t mm_alloc(void);

struct pmman pmman = {
    .free       = mm_free,
    .alloc      = mm_alloc,
    .mem_used   = mem_used,
    .mem_free   = mem_free,
    .get_page   = __page_alloc,
    .get_pages  = __page_alloc_n,
    .init       = physical_memory_init,
};

uintptr_t mm_alloc(void) {
    uintptr_t paddr = 0;
    __page_alloc(GFP_NORMAL, (void **)&paddr);
    return paddr;
}

void mm_free(uintptr_t addr) {
    __page_putref(addr);
}

size_t mem_free(void) {
    size_t size = 0;
    zone_t *z = NULL;

    for (size_t zone = ZONEi_DMA; zone < NELEM(zones); ++zone) {
        zone_lock(&zones[zone]);
        if (zone_isvalid(z = &zones[zone])) {
            size += (z->npages - z->upages) * PAGESZ;
        }
        zone_unlock(&zones[zone]);
    }
    return (size / 1024);
}

size_t mem_used(void) {
    size_t size = 0;

    for (size_t zone = ZONEi_DMA; zone < NELEM(zones); ++zone) {
        zone_lock(&zones[zone]);
        if (zone_isvalid(&zones[zone]))
            size += zones[zone].upages * PAGESZ;
        zone_unlock(&zones[zone]);
    }
    return (size / 1024);
}

#define page_index(page, zone)      ({ ((page) - (zone)->pages); })
#define page_addr(page, zone)       ({ (zone)->start + (page_index(page, zone) * PGSZ); })

int page_alloc_n(gfp_t gfp, usize order, page_t **pp) {
    int         err     = 0;
    int         tglocked= 0;
    int         tlocked = 0;
    int         whence  = 0;
    uintptr_t   paddr   = 0;
    void        *vaddr  = 0;
    usize       index   = 0;
    usize       start   = 0;
    usize       count   = 0;
    page_t      *page   = NULL;
    zone_t      *zone   = NULL;
    usize       npage   = BS(order);
    // pte_t       *p      = NULL;

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

        if ((BS(order) > (zone->npages - zone->upages))) {
            zone_unlock(zone);
            return -ENOMEM;
        }

        for (start = index = 0, page = zone->pages; index < zone->npages; ++index) {
            if (atomic_read(&page->refcnt)) {
                // start counting allocated pages anew.
                count   = 0;
                start   = index + 1;
                page    = &zone->pages[start];
                continue;
            }

            // managed to get all the pages?
            if (++count == npage) {
                for ( index++; page < &zone->pages[index]; page++) {
                    zone->upages++;
                    assert(!atomic_read(&page->refcnt),
                            "page->refcnt not 'zero'???.");

                    assert_msg(page_addr(page, zone) != zones[ZONEi_NORM].start,
                        "%s:%d: Page belongs to kernel, page: %p\n",
                        __FILE__, __LINE__, page_addr(page, zone)
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
                            bzero(vaddr, PGSZ);
                            // unmount the page frame.
                            arch_unmount((uintptr_t)vaddr);
                        } else {
                            /// addresses from 0->2GiB are indically mapped.
                            /// so just convert the paddr directly to vaddr.
                            vaddr = (void *)VMA2HI(paddr);
                            bzero(vaddr, PGSZ);
                        }
                    }
                }

                // set the return address and return after unlocking 'zone'.
                *pp = &zone->pages[start];
                zone_unlock(zone);
                return 0;
            }
        }

        zone_unlock(zone);

        // Not enough spage is available to satisfy the request.
        if (!(gfp & GFP_WAIT) && !(gfp & GFP_RETRY)) {
            /// thread ellected not to wait or retry the request
            /// so just return -ENOMEM.
            return -ENOMEM;
        }

        // caller requested to wait for memory availability?
        if ((gfp & GFP_WAIT)) {
            if (current == NULL) {
                // no thread currently running, just return -ENOMEM.
                return -ENOMEM;
            }

            if ((tglocked = !current_tgroup_islocked()))
                current_tgroup_lock();

            if ((tlocked = !current_islocked()))
                current_lock();

            err = sched_sleep_r(&zone->queue, T_ISLEEP, &zone->lock);

            if (tlocked)
                current_unlock();

            if (tglocked)
                current_tgroup_unlock();

            if (err) {
                return err;
            }

            continue;
        }

        if (!(gfp & GFP_RETRY)) {
            return -ENOMEM;
        }

        // No thread is running, so prevent indefinite loop by CPU.
        if (current == NULL) {
            return -ENOMEM;
        }
    }
}

int page_alloc(gfp_t gfp, page_t **pp) {
    return page_alloc_n(gfp, 0, pp);
}

void page_free_n(page_t *page, usize order) {
    int         err     = 0;
    usize       npage   = 0;
    zone_t      *zone   = NULL;

    assert(page, "Attemp to free NULL.");

    assert_msg(order < 64, "%s:%d: paddr: %p, "
        "Order(%d) requested is too large.",
        __FILE__, __LINE__, page, order);

    err = getzone_bypage(page, &zone);

    assert_msg(zone, "%s:%d: zone not "
        "found for paddr: %p, err = %d\n", __FILE__, __LINE__, page, err);

    assert(page_addr(page, zone) != zones[ZONEi_NORM].start, "Page belongs to kernel");

    npage   = BS(order);

    if (npage > zone->npages) {
        panic("%s:%d: How did we get "
            "access with npage out of bounds.", __FILE__, __LINE__);
        zone_unlock(zone);
        return;
    }

    if (&page[npage - 1] >= &zone->pages[zone->npages]) {
        panic("%s:%d: How did we get access "
            "with npage out of bounds.", __FILE__, __LINE__);
        zone_unlock(zone);
        return;
    }

    for (usize count = 0; count < npage; ++count, page++) {
        assert(atomic_read(&page->refcnt),"Page already free..??");
        if (atomic_dec_fetch(&page->refcnt) == 0) {
            page->virtual = 0;
            page->icache = NULL;
            
            page_resetflags(page);
            page_setswappable(page);
            zone->upages--;
        }
    }
    zone_unlock(zone);
}

void page_free(page_t *page) {
    page_free_n(page, 0);
}

int page_increment(page_t *page) {
    int         err     = 0;
    zone_t  *zone   = NULL;

    if (page == NULL)
        return -EINVAL;

    if ((err = getzone_bypage(page, &zone)))
        return err;
    
    atomic_inc(&page->refcnt);
    zone_unlock(zone);
    return 0;
}

int page_decrement(page_t *page) {
    int         err     = 0;
    zone_t  *zone   = NULL;

    if (page == NULL)
        return -EINVAL;

    if ((err = getzone_bypage(page, &zone)))
        return err;
    
    atomic_dec(&page->refcnt);
    zone_unlock(zone);
    return 0;
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

void __page_free_n(uintptr_t paddr, usize order) {
    usize       npage   = 0;
    page_t      *page   = NULL;
    zone_t      *zone   = NULL;

    assert(paddr, "Attemp to free NULL.");

    assert_msg(order < 64,
        "%s:%d: paddr: %p, Order(%d) requested is too large.",
        __FILE__, __LINE__, paddr, order);

    npage   = BS(order);

    getzone_byaddr(paddr, npage * PGSZ, &zone);

    assert_msg(zone, "%s:%d: zone not "
        "found for paddr: %p\n", __FILE__, __LINE__, paddr);

    if (npage > zone->npages) {
        panic("%s:%d: How did we get access with npage out of bounds.", __FILE__, __LINE__);
        zone_unlock(zone);
        return;
    }

    page = &zone->pages[(paddr - zone->start) / PGSZ];
    
    assert(page_addr(page, zone) != zones[ZONEi_NORM].start, "Page belongs to kernel");
    
    for (usize count = 0; count < npage; ++count, page++) {
        assert(atomic_read(&page->refcnt),"Page already free..??");
        if (atomic_dec_fetch(&page->refcnt) == 0) {
            page->virtual = 0;
            page->icache = NULL;
            
            page_resetflags(page);
            page_setswappable(page);
            zone->upages--;
        }
    }
    zone_unlock(zone);
    return;
}

void __page_free(uintptr_t paddr) {
    __page_free_n(paddr, 0);
}

int __page_increment(uintptr_t paddr) {
    int     err     = 0;
    zone_t  *zone   = NULL;

    if (!paddr)
        return -EINVAL;

    if ((err = getzone_byaddr(paddr, PGSZ, &zone)))
        return err;

    atomic_inc(&zone->pages[(paddr - zone->start) / PGSZ].refcnt);
    zone_unlock(zone);
    return 0;
}

int __page_decrement(uintptr_t paddr) {
    int     err     = 0;
    zone_t  *zone   = NULL;

    if (!paddr)
        return -EINVAL;

    if ((err = getzone_byaddr(paddr, PGSZ, &zone)))
        return err;

    atomic_dec(&zone->pages[(paddr - zone->start) / PGSZ].refcnt);
    zone_unlock(zone);
    return 0;
}

int page_getref(page_t *page) {
    return page_increment(page);
}

int __page_getref(uintptr_t paddr) {
    return __page_increment(paddr);
}

void page_putref(page_t *page) {
    int err = 0;

    assert_msg((err = page_decrement(page)) == 0,
                "%s:%d: Failed to decrement page: %p, err: %d\n",
                __FILE__, __LINE__, page, err);
}

int __page_putref(uintptr_t paddr) {
    return __page_decrement(paddr);
}

int page_get_address(page_t *page, void **ppa) {
    int     err = 0;
    zone_t  *z  = NULL;

    if (page == NULL || ppa == NULL)
        return -EINVAL;
    
    if ((err = getzone_bypage(page, &z)))
        return err;
    
    // convert the page to a physical address.
    *ppa = (void *)((page_index(page, z) * PGSZ) + z->start);
    // or simply: *ppa = page->phys_addr; ?

    zone_unlock(z);
    return 0;
}

int page_getcount(page_t *page, usize *pcnt) {
    int     err = 0;
    zone_t  *z  = NULL;

    if (page == NULL || pcnt == NULL)
        return -EINVAL;

    if ((err = getzone_bypage(page, &z)))
        return err;
    
    *pcnt = atomic_read(&page->refcnt);
    zone_unlock(z);
    return 0;
}

int __page_getcount(uintptr_t paddr, usize *pcnt) {
    int     err = 0;
    zone_t  *zone   = NULL;

    if (!paddr || pcnt == NULL)
        return -EINVAL;

    if ((err = getzone_byaddr(paddr, PGSZ, &zone)))
        return err;

    *pcnt = atomic_read(&zone->pages[(paddr - zone->start) / PGSZ].refcnt);
    zone_unlock(zone);
    return 0;
}