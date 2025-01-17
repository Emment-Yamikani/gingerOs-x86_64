#include <arch/paging.h>
#include <bits/errno.h>
#include <boot/boot.h>
#include <lib/string.h>
#include <mm/zone.h>
#include <sys/system.h>
#include <core/debug.h>

zone_t zones[NZONE];

const char *str_zone[] = {
    "DMA", "NORM", "HOLE", "HIGH", NULL,
};

void zone_dump(zone_t *zone) {
    assert(zone, "zerror: No physical memory zone specified\n");
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
    
    for (zone_t *zone = zones; zone < &zones[NZONE]; ++zone) {
        zone_lock(zone);
        if ((paddr >= zone->start) && zone_isvalid(zone) &&
                ((paddr + size) <= (zone->start + zone->size))) {
            *ppz = zone;
            return 0;
        }
        zone_unlock(zone);
    }

    return -ENOENT;
}

int getzone_bypage(page_t *page, zone_t **ppz) {
    if (page == NULL || ppz == NULL)
        return -EINVAL;
    
    for (zone_t *zone = zones; zone < &zones[NZONE]; ++zone) {
        zone_lock(zone);
        if ((page >= zone->pages) && zone_isvalid(zone) &&
                (page < &zone->pages[zone->npages])) {
            *ppz = zone;
            return 0;
        }
        zone_unlock(zone);
    }

    return -ENOENT;
}

int getzone_byindex(int zone_i, zone_t **ref) {
    zone_t *zone = NULL;

    if ((zone_i < 0) || (zone_i > (int)NELEM(zones)))
        return -EINVAL;

    zone = &zones[zone_i];
    zone_lock(zone);
    // printk("requesting zone(%d): %p ,flags: %d\n", zone_i, zone, zone->flags);
    if (!zone_isvalid(zone)) {
        zone_unlock(zone);
        return -EINVAL;
    }

    *ref = zone;
    return 0;
}

/**
 * Initialize a memory zone based on the available memory.
 *
 * @param zone Pointer to the zone to initialize.
 * @param memsz Pointer to the remaining memory size in KiB.
 * @return 0 on success, -EINVAL if the previous zone isn't valid.
 */
static int zone_enumerate(zone_t *zone, usize *memsz) {
    int         err  = 0;

    if (zone == NULL)
        return -EINVAL;

    zone_assert_locked(zone);

    debug("Initializing zone %s: remaining memory size %lu KiB.\n", str_zone[zone - zones], *memsz);

    switch (zone - zones) {
    case ZONEi_DMA:
        if (*memsz > M2KiB(16)) {
            zone->size = MiB(16);
        } else zone->size = KiB(*memsz);

        zone->start = 0; // DMA zone starts at 0x0
        break;
    case ZONEi_NORM:
        // enure that the previous zone is initialize before this one.
        if (!((zone - 1)->flags & ZONE_VALID))
            return -EINVAL;

        /// manipulate the available memory size.
        /// if available memory size is large split it.
        /// else use as is.
        /// memsz is in KiB.
        if (*memsz > M2KiB(2032)) {
            zone->size = MiB(2032);
        } else zone->size = KiB(*memsz);

        /// set this zone starts @ 16MiB.
        zone->start = MiB(16);

        break;
    case ZONEi_HOLE:
        // enure that the previous zone is initialize before this one.
        if (!((zone - 1)->flags & ZONE_VALID))
            return -EINVAL;

        /** We add 1 MiB because multiboot says;
         * "The value returned for upper memory is
         * maximally the address of the first
         * upper memory hole minus 1 megabyte.
         * It is not guaranteed to be this value".*/
        zone->size = (KiB((bootinfo.memhi + M2KiB(1))) + bootinfo.hole_size) - GiB(2);

        /// set this zone starts @ 2GiB.
        zone->start = GiB(2);

        break;
    case ZONEi_HIGH:
        // enure that the previous zone is initialize before this one.
        if (!((zone - 1)->flags & ZONE_VALID))
            return -EINVAL;

        /// for HIGH memory zone, no spliting is needed.
        zone->size     = KiB(*memsz);

        /// set this zone starts @ 4GiB.
        zone->start = GiB(4);

        break;
    default: return -EINVAL;
    }

    zone->npages   = NPAGE(zone->size);

    if (zone_size(zone) != 0) {
        usize bitmap_u64s   = 0;
        usize *bitmap_array = NULL;
        size_t size = 0;

        bitmap_u64s = ((zone->npages + BITS_PER_USIZE - 1) / BITS_PER_USIZE);

        size = bitmap_u64s * sizeof(usize);
        size += zone->npages * sizeof(page_t);
        size = NPAGE(size) * PGSZ;

        bitmap_array = boot_alloc(size, PGSZ);

        if ((err = bitmap_init(&zone->bitmap, bitmap_array, zone->npages)))
            return err;

        zone->pages = (page_t *)&bitmap_array[bitmap_u64s];

        // mark zone as valid for use.
        zone_flags_set(zone, ZONE_VALID);
        // clear the page array.
        memset(zone->pages, 0, sizeof(page_t) * zone->npages);

    }

    *memsz -= B2KiB(zone->size);

    debug("Initialized zone %s[%p: %X]: remaining memory size %lu KiB.\n",
          str_zone[zone - zones], zone->start, zone->size, *memsz);

    return 0;
}

static void initialize_zone_struct(zone_t *zone) {
    memset(zone, 0, sizeof(*zone));

    zone->queue     = QUEUE_INIT();
    zone->bitmap    = BITMAP_INIT();
    zone->lock      = SPINLOCK_INIT();
}

// Helper function to initialize individual zones.
static int initialize_memory_zone(zone_t *zone, usize *memsz) {
    int err;

    initialize_zone_struct(zone);
    zone_lock(zone);

    if (*memsz != 0) {
        if ((err = zone_enumerate(zone, memsz))) {
            printk("Failed to init zone. err: %d\n", err);
            zone_unlock(zone);
            return err;
        }
    }

    zone_unlock(zone);
    return 0;
}

// Helper function to process pages within a memory range
static int process_pages(zone_t *zone, uintptr_t addr, usize size) {
    usize   np      = 0;
    page_t  *page   = NULL;

    if (zone == NULL)
        return -EINVAL;

    page = zone->pages + NPAGE(addr - zone->start);
    for (np = NPAGE(size); np; --np, page++, addr += PGSZ) {
        bitmap_set(&zone->bitmap, page - zone->pages, 1);

        if (page->refcnt == 0) {
            zone->upages += 1; // Increment number of used pages.
        } else {
            printk("%s(): %s:%d: [NOTE]: page(%p)->refcnt == %d?\n",
                   __func__, __FILE__, __LINE__, addr, page->refcnt);
        }

        page->refcnt += 1;
        page->mapcnt += 1;
        page_setflags(page, PG_R | PG_X); // Read and exec only.
        page->virtual = V2HI(addr);
    }

    return 0;
}

// Helper function to process a memory map region.
static int process_memory_map(boot_mmap_t *map) {
    int         err     = 0;
    uintptr_t   addr    = 0;
    usize       size    = 0;
    zone_t      *zone   = NULL;

    // Skip available regions or regions outside the physical memory.
    if ((map->addr >= V2HI(MEMMIO)) || (map->type == MULTIBOOT_MEMORY_AVAILABLE))
        return 0;

    size = map->size;
    addr = PGROUND(V2LO(map->addr));

    if ((err = getzone_byaddr(addr, size, &zone)))
        panic("PANIC: %s(): %s:%d: Couldn't get zone for mmap[%p: %d], err: %d\n",
            __func__, __FILE__, __LINE__, addr, size, err);

    if ((err = process_pages(zone, addr, size)))
        panic("PANIC: %s(): %s:%d: Couldn't process memory region[%p: %ld]. err: %d\n",
            __func__, __FILE__, __LINE__, addr, size, err);

    zone_unlock(zone);
    return 0;
}

// Helper function to process a module region.
static int process_module(mod_t *mod) {
    int         err     = 0;
    uintptr_t   addr    = 0;
    usize       size    = 0;
    zone_t      *zone   = NULL;

    size = mod->size;
    addr = PGROUND(V2LO(mod->addr));

    if ((err = getzone_byaddr(addr, size, &zone)))
        panic("PANIC: %s(): %s:%d: Couldn't get zone for module region[%p: %ld], err: %d\n",
            __func__, __FILE__, __LINE__, addr, size, err);

    if ((err = process_pages(zone, addr, size)))
        panic("PANIC: %s(): %s:%d: Couldn't process module region[%p: %ld]. err: %d\n",
            __func__, __FILE__, __LINE__, addr, size, err);

    zone_unlock(zone);
    return 0;
}

// Main zones_init function.
int zones_init(void) {
    int         err     = 0;
    boot_mmap_t *map    = NULL;
    mod_t       *mod    = NULL;
    zone_t      *zone   = NULL;
    usize       memsz   = bootinfo.total;

    printk("Initializing memory zones...\n");

    // Initialize zones.
    for (zone = zones; zone < &zones[NZONE]; ++zone) {
        if ((err = initialize_memory_zone(zone, &memsz)))
            return err;
    }

    // Process memory map regions.
    for (map = bootinfo.mmap; map < &bootinfo.mmap[bootinfo.mmapcnt]; ++map) {
        if ((err = process_memory_map(map)))
            return err;
    }

    // Process module regions.
    for (mod = bootinfo.mods; mod < &bootinfo.mods[bootinfo.modcnt]; ++mod) {
        if ((err = process_module(mod)))
            return err;
    }

    printk("Memory zones initialized.\n");
    return 0;
}


int physical_memory_init(void) {
    int         err  = 0;
    usize       size = 0;
    uintptr_t   addr = 0;
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

    arch_map_i(
        bootinfo.fb.addr,
        V2LO(bootinfo.fb.addr),
        bootinfo.fb.size, PTE_KRW | PTE_WTCD
    );

    return 0;
}