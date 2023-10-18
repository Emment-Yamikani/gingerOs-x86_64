#include <mm/pmm.h>
#include <mm/vmm.h>
#include <mm/mm_zone.h>
#include <sys/system.h>
#include <bits/errno.h>
#include <lib/string.h>
#include <sys/thread.h>
#include <sync/atomic.h>
#include <arch/x86_64/pml4.h>

size_t mem_free(void);
size_t mem_used(void);
void mm_free(uintptr_t);
uintptr_t mm_alloc(void);

struct pmman pmman = {
    .free = mm_free,
    .alloc = mm_alloc,
    .mem_used = mem_used,
    .mem_free = mem_free,
    .get_page = __get_free_page,
    .get_pages = __get_free_pages,
    .init = physical_memory_init,
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
            if (zone->pages[index].ref_count) {
                alloced = 0;
                start = index + 1;
                continue;
            }

            // printk("%s index: %d, count: %d, needed: %d\n", str_zone[zone - zones], index, zone->pages[index].ref_count, npages);
            if ((++alloced) == npages)
                goto done;
        }
        if ((!(gfp & GFP_WAIT) && !(gfp & GFP_RETRY)))
            goto error;
    }

done:
    page = &zone->pages[start];
    for (size_t count = 0; count < npages; ++count) {
        page[count].flags.mm_zone = zone - zones;
        page[count].ref_count++;
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
                vaddr = page_mount(paddr);
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
                    page_unmount((uintptr_t)vaddr);
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

    if (!(zone = mm_zone_get(page->flags.mm_zone)))
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

int __page_incr(uintptr_t addr) {
    size_t refcnt = 0;
    mm_zone_t *zone = NULL;

    if (!addr)
        panic("%s(%p)???\n", __func__, addr);
    if (!(zone = get_mmzone(addr, PAGESZ)))
        return -EADDRNOTAVAIL;
    refcnt = atomic_inc(&zone->pages[(addr - zone->start) / PAGESZ].ref_count);
    mm_zone_unlock(zone);
    return refcnt;
}

int page_incr(page_t *page) {
    return __page_incr(page_address(page));
}

int __page_count(uintptr_t addr) {
    size_t refcnt = 0;
    mm_zone_t *zone = NULL;

    if (!addr)
        panic("%s(%p)???\n", __func__, addr);
    if (!(zone = get_mmzone(addr, PAGESZ)))
        return -EADDRNOTAVAIL;
    refcnt = atomic_read(&zone->pages[(addr - zone->start) / PAGESZ].ref_count);
    mm_zone_unlock(zone);
    return refcnt;
}

int page_count(page_t *page) {
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
        if (atomic_read(&page->ref_count) == 0)
            continue;
        atomic_dec(&page->ref_count);
        if (atomic_read(&page->ref_count) == 0) {
            page->mapping = NULL;
            page->ref_count = 0;
            page->virtual = 0;
            page->flags.read = 0;
            page->flags.can_swap = 1;
            page->flags.dirty = 0;
            page->flags.exec = 0;
            page->flags.shared = 0;
            page->flags.swapped = 0;
            page->flags.user = 0;
            page->flags.valid = 0;
            page->flags.write = 0;
            page->flags.writeback =0;
            zone->free_pages++;
        }
    }
    mm_zone_unlock(zone);
}

void __page_put(uintptr_t addr) {
    __pages_put(addr, 0);
}

uintptr_t page_mount(uintptr_t paddr) {
    int err = 0;
    uintptr_t vaddr = 0;

    if ((paddr == 0) || !(vaddr = vmman.alloc(PGSZ)))
        return 0;

    pagemap_binary_lock(&kernel_map);
    err = map_page_to(&kernel_map, vaddr, paddr, VM_KRW);
    pagemap_binary_unlock(&kernel_map);

    if (err) {
        vmman.free(vaddr);
        return 0;
    }

    return vaddr;
}

void page_unmount(uintptr_t vaddr) {
    pagemap_binary_lock(&kernel_map);
    unmap_page(&kernel_map, vaddr);
    pagemap_binary_unlock(&kernel_map);
    vmman.free(vaddr);
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
        if (mm_zone_isvalid(&zones[zone])){
            size += zones[zone].free_pages * PAGESZ;
            printk("size: %ld\n", size);
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