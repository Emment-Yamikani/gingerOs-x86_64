#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/mm_zone.h>
#include <sys/system.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <sys/thread.h>
#include <sync/atomic.h>
#include <arch/paging.h>

size_t mem_free(void);
size_t mem_used(void);
void mm_free(uintptr_t);
uintptr_t mm_alloc(void);

struct pmman pmman = {
    .free       = mm_free,
    .alloc      = mm_alloc,
    .mem_used   = mem_used,
    .mem_free   = mem_free,
    .get_page   = __get_free_page,
    .get_pages  = __get_free_pages,
    .init       = physical_memory_init,
};

page_t *alloc_pages(gfp_mask_t gfp, size_t order) {
    size_t index = 0;
    size_t start = 0;
    page_t *page = NULL;
    uintptr_t paddr = 0;
    uintptr_t vaddr = 0;
    mm_zone_t *zone = NULL;
    size_t alloced = 0, npages = BS(order);
    int where = !(gfp & 0x0F) ? MM_ZONE_NORM : (gfp & 0x0F) - 1;

    zone = mm_zone_get(where);
    if (!zone)
        return NULL;
    // printk("trying allocation from zone: %d: npages: %d\n", where, zone->free_pages);
    // printk("getting ppage frame...\n");
    loop() {
        for (start = index = 0; index < zone->nrpages; ++index) {
            if (atomic_read(&zone->pages[index].pg_refcnt)) {
                alloced = 0;
                start = index + 1;
                continue;
            }

            // printk("%s index: %d, count: %d, needed: %d\n", str_zone[zone - zones], index, zone->pages[index].pg_refcnt, npages);
            if ((++alloced) == npages)
                goto done;
        }
        if ((!(gfp & GFP_WAIT) && !(gfp & GFP_RETRY)))
            goto error;
    }

done:
    page = &zone->pages[start];
    for (size_t count = 0; count < npages; ++count) {
        page_setmmzone(&page[count], zone - zones);
        atomic_inc(&page[count].pg_refcnt);
        zone->free_pages--;
    }

    if (gfp & GFP_ZERO) {
    // zero out the page frame(s)
        for (size_t count = 0; count < npages; ++count) {
            paddr = zone->start + ((&page[count] - zone->pages) * PAGESZ);
            if (where == MM_ZONE_HIGH) {
                printk("may need to properly sleep\n");
            map:
                if (current)
                    goto map;
                arch_mount(paddr, (void **)&vaddr);
                if (vaddr == 0) {
                    if (current)
                        goto map;
                    if (current) {
                        current_lock();
                        sched_sleep(mm_zone_sleep_queue[where], T_ISLEEP, &zone->lock);
                        current_unlock();
                        goto map;
                    }
                } else {
                    memset((void *)vaddr, 0, PAGESZ);
                    arch_unmount((uintptr_t)vaddr);
                }
            } else {
                memset((void *)VMA2HI(paddr), 0, PAGESZ);
            }
        }
    }
    mm_zone_unlock(zone);
    return page;
error:
    mm_zone_unlock(zone);
    return NULL;
}

page_t *alloc_page(gfp_mask_t gfp) {
    return alloc_pages(gfp, 0);
}

uintptr_t page_address(page_t *page) {
    long index = 0;
    uintptr_t addr = 0;
    mm_zone_t *zone = NULL;

    if (!page)
        return 0;

    if (!(zone = mm_zone_get(page_getmmzone(page))))
        return 0;

    index = page - zone->pages;

    if ((index < 0) || (index > (long)zone->nrpages)) {
        mm_zone_unlock(zone);
        return 0;
    }

    addr = zone->start + (index * PAGESZ);
    mm_zone_unlock(zone);
    return addr;
}

size_t __page_incr(uintptr_t addr) {
    size_t pg_refcnt = 0;
    mm_zone_t *zone = NULL;

    if (!addr)
        panic("%s(%p)???\n", __func__, addr);
    if (!(zone = get_mmzone(addr, PAGESZ)))
        return -EADDRNOTAVAIL;
    pg_refcnt = atomic_inc(&zone->pages[(addr - zone->start) / PAGESZ].pg_refcnt);
    mm_zone_unlock(zone);
    return pg_refcnt;
}

size_t page_incr(page_t *page) {
    return __page_incr(page_address(page));
}

size_t __page_count(uintptr_t addr) {
    size_t pg_refcnt = 0;
    mm_zone_t *zone = NULL;

    if (!addr)
        panic("%s(%p)???\n", __func__, addr);
    if (!(zone = get_mmzone(addr, PAGESZ)))
        return -EADDRNOTAVAIL;
    pg_refcnt = atomic_read(&zone->pages[(addr - zone->start) / PAGESZ].pg_refcnt);
    mm_zone_unlock(zone);
    return pg_refcnt;
}

size_t page_count(page_t *page) {
    return __page_count(-page_address(page));
}

uintptr_t __get_free_pages(gfp_mask_t gfp, size_t order) {
    return page_address(alloc_pages(gfp, order));
}

uintptr_t __get_free_page(gfp_mask_t gfp) {
    return __get_free_pages(gfp, 0);
}

void pages_put(page_t *page, size_t order) {
    __pages_put(page_address(page), order);
}

void page_put(page_t *page) {
    pages_put(page, 0);
}

void __pages_put(uintptr_t addr, size_t order) {
    page_t *page = NULL;
    mm_zone_t *zone = NULL;
    size_t npages = BS(order);

    if (!addr)
        panic("%s(%p, %d)???\n", __func__, addr, order);

    if (!(zone = get_mmzone(addr, npages * PAGESZ)))
        return;
    
    page = &zone->pages[(addr - zone->start) / PAGESZ];
    for (size_t pages = 0; pages < npages; ++pages, ++page) {
        if (atomic_read(&page->pg_refcnt) == 0)
            continue;
        atomic_dec(&page->pg_refcnt);
        if (atomic_read(&page->pg_refcnt) == 0) {
            page->pg_virtual = 0;
            page_resetflags(page);
            page_setswappable(page);
            page->pg_mapping = NULL;
            atomic_write(&page->pg_refcnt, 0);
            zone->free_pages++;
        }
    }
    mm_zone_unlock(zone);
}

void __page_put(uintptr_t addr) {
    __pages_put(addr, 0);
}

uintptr_t mm_alloc(void) {
    return __get_free_page(GFP_NORMAL);
}

void mm_free(uintptr_t addr) {
    //printk("%s(%p)\n", __func__, addr);
    __page_put(addr);
}

size_t mem_free(void) {
    size_t size = 0;
    for (size_t zone = MM_ZONE_DMA; zone < NELEM(zones); ++zone) {
        mm_zone_lock(&zones[zone]);
        if (mm_zone_isvalid(&zones[zone])) {
            size += zones[zone].free_pages * PAGESZ;
        }
        mm_zone_unlock(&zones[zone]);
    }
    return (size / 1024);
}

size_t mem_used(void) {
    size_t size = 0;
    for (size_t zone = MM_ZONE_DMA; zone < NELEM(zones); ++zone) {
        mm_zone_lock(&zones[zone]);
        if (mm_zone_isvalid(&zones[zone]))
            size += (zones[zone].nrpages - zones[zone].free_pages) * PAGESZ;
        mm_zone_unlock(&zones[zone]);
    }
    return (size / 1024);
}

void *memory(void *arg) {
    
    return arg;
}

// BUILTIN_THREAD(memory, memory, NULL);