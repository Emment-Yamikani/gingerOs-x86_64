#include <arch/paging.h>
#include <bits/errno.h>
#include <boot/boot.h>
#include <lib/string.h>
#include <mm/zone.h>
#include <sys/system.h>

zone_t zones[NZONE];

const char *str_zone[] = {
    "DMA", "NORM", "HOLE", "HIGH", NULL,
};

void zone_dump(zone_t *zone) {
    assert_msg(zone, "zerror: No physical "
        "memory zone specified\n", __FILE__, __LINE__);
    printk("\nZONE: %s\n"
            "Array:  %16p\n"
            "Size:   %16d KiB\n"
            "Free:   %16d pages\n"
            "Used:   %16d pages\n"
            "Total:  %16d pages\n"
            "Start:  %16p\n"
            "End:    %16p\n",
            str_zone[zone - zones],
            zone->pages,
            zone->size / KiB(1),
            zone->npages - zone->upages,
            zone->upages,
            zone->npages,
            zone->start,
            zone->start + zone->size
    );
}

int getzone_byaddr(uintptr_t paddr, usize size, zone_t **ppz) {
    if (ppz == NULL)
        return -EINVAL;
    
    for (zone_t *z = zones; z < &zones[NZONE]; ++z) {
        zone_lock(z);
        if ((paddr >= z->start) && zone_isvalid(z) &&
                ((paddr + size) <= (z->start + z->size))) {
            *ppz = z;
            return 0;
        }
        zone_unlock(z);
    }

    return -ENOENT;
}

int getzone_bypage(page_t *page, zone_t **ppz) {
    if (page == NULL || ppz == NULL)
        return -EINVAL;
    
    for (zone_t *z = zones; z < &zones[NZONE]; ++z) {
        zone_lock(z);
        if ((page >= z->pages) && zone_isvalid(z) &&
                (page < &z->pages[z->npages])) {
            *ppz = z;
            return 0;
        }
        zone_unlock(z);
    }

    return -ENOENT;
}

int getzone_byindex(int z, zone_t **ref) {
    zone_t *zone = NULL;

    if ((z < 0) || (z > (int)NELEM(zones)))
        return -EINVAL;

    zone = &zones[z];
    zone_lock(zone);
    // printk("requesting zone(%d): %p ,flags: %d\n", z, zone, zone->flags);
    if (!zone_isvalid(zone)) {
        zone_unlock(zone);
        return -EINVAL;
    }

    *ref = zone;
    return 0;
}

static int zone_enumerate(zone_t *z, usize *memsz) {
    page_t *pages = (page_t *)V2HI(bootinfo.phyaddr);

    if (z == NULL)
        return -EINVAL;

    zone_assert_locked(z);

    switch (z - zones) {
    case ZONEi_DMA:
        if (*memsz > M2KiB(16)) {
            z->size = MiB(16);
        } else z->size = KiB(*memsz);

        z->start = 0; // DMA zone starts at 0x0
        break;
    case ZONEi_NORM:
        // enure that the previous zone is initialize before this one.
        if (!((z - 1)->flags & ZONE_VALID))
            return -EINVAL;

        /// manipulate the available memory size.
        /// if available memory size is large split it.
        /// else use as is.
        /// memsz is in KiB.
        if (*memsz > M2KiB(2032)) {
            z->size = MiB(2032);
        } else z->size = KiB(*memsz);

        /// set this zone starts @ 16MiB.
        z->start = MiB(16);

        break;
    case ZONEi_HOLE:
        // enure that the previous zone is initialize before this one.
        if (!((z - 1)->flags & ZONE_VALID))
            return -EINVAL;

        /** We add 1 MiB because multiboot says;
         * "The value returned for upper memory is
         * maximally the address of the first
         * upper memory hole minus 1 megabyte.
         * It is not guaranteed to be this value".*/
        z->size = KiB((bootinfo.memhi + M2KiB(1))) - GiB(2);

        /// set this zone starts @ 2GiB.
        z->start = GiB(2);

        break;
    case ZONEi_HIGH:
        // enure that the previous zone is initialize before this one.
        if (!((z - 1)->flags & ZONE_VALID))
            return -EINVAL;

        /// for HIGH memory zone, no spliting is needed.
        z->size     = KiB(*memsz);

        /// set this zone starts @ 4GiB.
        z->start = GiB(4);

        break;
    default: return -EINVAL;
    }

    z->npages   = NPAGE(z->size);

    if (zone_size(z) != 0) {
        z->pages    = pages;
        // mark zone as valid for use.
        zone_flags_set(z, ZONE_VALID);
        // clear the page array.
        memset(z->pages, 0, sizeof(page_t) * z->npages);
    }

    *memsz -= B2KiB(z->size);
    bootinfo.phyaddr = V2LO((pages + z->npages));

    return 0;
}

int zones_init(void) {
    int         err     = 0;    // error code.
    uintptr_t   addr    = 0;    // address of memory region.
    usize       size    = 0;    // size of memory region in bytes.
    usize       np      = 0;    // no. of pages.
    page_t      *page   = NULL;
    zone_t      *z      = NULL; // memory zone.
    mod_t       *mod    = bootinfo.mods;
    boot_mmap_t *map    = bootinfo.mmap;
    usize       memsz   = bootinfo.total;

    printk("initializing memory zones...\n");

    for (z = zones; z < &zones[NZONE]; ++z) {
        memset(z, 0, sizeof *z);
        z->lock = SPINLOCK_INIT();
        
        zone_lock(z);
        if (memsz != 0) {
            if ((err = zone_enumerate(z, &memsz))) {
                printk("Failed to init zone. err: %d\n", err);
                zone_unlock(z);
                return err;
            }
        }
        zone_unlock(z);
    }

    for (z = NULL; map < &bootinfo.mmap[bootinfo.mmapcnt]; ++map) {
        // only mark the regions that aren't available as 'used'.
        if ((map->addr < V2HI(MEMMIO)) &&
                (map->type != MULTIBOOT_MEMORY_AVAILABLE)) {
            size = map->size;
            addr = PGROUND(V2LO(map->addr));

            if ((err = getzone_byaddr(addr, size, &z)))
                panic("Couldn't get zone for mmap[%p, %d], err: %d\n", addr, size, err);

            page = z->pages + NPAGE(addr - z->start);
            for (np = NPAGE(size); np; --np, page++, addr += PGSZ) {
                if (page->refcnt == 0)
                    z->upages       += 1; // increment no. used pages.
                else printk("%s:%d: [NOTE]: already marked!!!\n", __FILE__, __LINE__);

                page->refcnt    += 1;
                page->mapcnt    += 1;
                // page->phys_addr = addr;
                page_setflags(page, PG_R | PG_X); // read and exec only.
                page->virtual   = V2HI(addr);

            }
            zone_unlock(z);
        }
    }

    for (z = NULL; mod < &bootinfo.mods[bootinfo.modcnt]; ++mod) {
        size = mod->size;
        addr = PGROUND(V2LO(mod->addr));

        if ((err = getzone_byaddr(addr, size, &z)))
            panic("Couldn't get zone, err: %d\n", err);

        page = z->pages + NPAGE(addr - z->start);
        for (np = NPAGE(size); np; --np, page++, addr += PGSZ) {
            if (page->refcnt == 0)
                z->upages       += 1; // increment no. used pages.
            else printk("%s:%d: [NOTE]: already marked!!!\n", __FILE__, __LINE__);

            page->refcnt    += 1;
            page->mapcnt    += 1;
            // page->phys_addr = addr;
            page_setflags(page, PG_R | PG_X); // read and exec only.
            page->virtual   = V2HI(addr);

        }

        zone_unlock(z);
    }

    printk("Memory zones initialized.\n");
    return 0;
}

int physical_memory_init(void) {
    int err = 0;
    usize size = 0;
    uintptr_t addr = 0;
    boot_mmap_t *map = bootinfo.mmap;

    if ((err = zones_init()))
        return 0;
    
    size = GiB(2) - PGROUNDUP(zones[ZONEi_NORM].size);

    if ((long)size > 0) {
        addr = PGROUNDUP(zone_end(&zones[ZONEi_NORM]));
        arch_unmap_n(V2HI(addr), size);
    }

    for (usize i = 0; i < bootinfo.mmapcnt; ++i) {
        addr = PGROUND(map[i].addr);
        size = PGROUNDUP(map[i].size);

        if ((V2LO(map[i].addr) < GiB(4)) &&
                (map[i].type != MULTIBOOT_MEMORY_AVAILABLE)) {
            uint32_t flags = PTE_KRW | PTE_WTCD;
            arch_map_i(addr, V2LO(addr), size, flags);
        }
    }

    arch_map_i(bootinfo.fb.addr, V2LO(bootinfo.fb.addr),
        bootinfo.fb.size, PTE_KRW | PTE_WTCD);

    return 0;
}