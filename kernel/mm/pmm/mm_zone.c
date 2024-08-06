#include <mm/mm_zone.h>
#include <mm/page.h>
#include <bits/errno.h>
#include <sys/system.h>
#include <lib/printk.h>
#include <mm/pmm.h>
#include <boot/boot.h>
#include <arch/paging.h>
#include <lib/string.h>
#include <lib/types.h>

mm_zone_t zones[NZONE] = {0};
const char *str_zone[] = {
    "DMA", "NORM", "HOLE", "HIGH", NULL,
};

queue_t *mm_zone_sleep_queue[] = {
    QUEUE_NEW(/*"MM_ZONE_DMA"*/),
    QUEUE_NEW(/*"MM_ZONE_NORM"*/),
    QUEUE_NEW(/*"MM_ZONE_HOLE"*/),
    QUEUE_NEW(/*"MM_ZONE_HIGH"*/),
};

static __unused void zone_dump(mm_zone_t *zone) {
    assert_msg(zone, "zerror: No physical memory zone specified\n", __FILE__, __LINE__);
    printk("\nStart: %p\nSize: %ld B\nSize: %d MiB\nfree_pages: %d\nnrpages: %d\nPages: %p\n",
           zone->start, zone->size, zone->size / MiB(1), zone->free_pages, zone->nrpages, zone->pages);
}

static int enumerate_zones(void) {
    void        *pages      = NULL;
    uintptr_t   start       = 0x0;
    usize      zones_size   = 0x0;
    usize      size         = MiB(16);
    usize      memsize      = KiB(bootinfo.memsize);
    mod_t       *last_mod   = &bootinfo.mods[bootinfo.modcnt - 1];

    zones_size = NPAGE(size) * sizeof(page_t);
    pages = (void *)PGROUNDUP((last_mod->addr + last_mod->size));

    printk("enumerating DMA memory zone...\n");

    zones[MM_ZONE_DMA] = (mm_zone_t){
        .pages      = pages,
        .nrpages    = size / PGSZ,
        .flags      = MM_ZONE_VALID,
        .lock       = SPINLOCK_INIT(),
        .start      = PGROUNDUP(start),
        .free_pages = size / PGSZ,
        .size       = size,
    };

    start       = MiB(16);
    memsize     -= MiB(16);
    size        = GiB(2) - MiB(16);
    size        = MIN(memsize, size);
    memsize     -= size;
    zones_size  = NPAGE(size) * sizeof(page_t);
    pages       += NPAGE(size) * sizeof(page_t);

    printk("enumerating NORM memory zone...\n");

    zones[MM_ZONE_NORM] = (mm_zone_t) {
        .pages      = pages,
        .nrpages    = size / PGSZ,
        .flags      = MM_ZONE_VALID,
        .lock       = SPINLOCK_INIT(),
        .start      = PGROUNDUP(start),
        .free_pages = size / PGSZ,
        .size       = size,
    };

    if (!memsize || !bootinfo.memhigh)
        goto done;

    printk("enumerating HOLE memory zone...\n");

    start       = GiB(2);
    size        = (KiB(bootinfo.memhigh) + MiB(1)) - GiB(2);
    size        = MIN(memsize, size);
    memsize     -= size;
    zones_size  = NPAGE(size) * sizeof(page_t);
    pages       += NPAGE(size) * sizeof(page_t);


    zones[MM_ZONE_HOLE] = (mm_zone_t){
        .pages      = pages,
        .nrpages    = size / PGSZ,
        .flags      = MM_ZONE_VALID,
        .lock       = SPINLOCK_INIT(),
        .start      = PGROUNDUP(start),
        .free_pages = size / PGSZ,
        .size       = size,
    };

    if (!memsize)
        goto done;

    printk("enumerating HIGH memory zone...\n");

    start       = GiB(4);
    size        = memsize;
    zones_size  = NPAGE(size) * sizeof(page_t);
    pages       += NPAGE(size) * sizeof(page_t);

    zones[MM_ZONE_HIGH] = (mm_zone_t) {
        .pages      = pages,
        .nrpages    = size / PGSZ,
        .flags      = MM_ZONE_VALID,
        .lock       = SPINLOCK_INIT(),
        .start      = PGROUNDUP(start),
        .free_pages = size / PGSZ,
        .size       = size,
    };

done:
    pages = zones[MM_ZONE_DMA].pages;
    memset(pages, 0, zones_size);
    return 0;
}

mm_zone_t *get_mmzone(uintptr_t addr, usize size) {
    for (usize i = 0; i < NELEM(zones); ++i) {
        if ((addr >= zones[i].start) && ((addr + (size - 1)) < (zones[i].start + zones[i].nrpages * PAGESZ))) {
            mm_zone_lock(&zones[i]);
            if (mm_zone_isvalid(&zones[i]))
                return &zones[i];
            else {
                mm_zone_unlock(&zones[i]);
                return NULL;
            }
        }
    }
    return NULL;
}

mm_zone_t *mm_zone_get(int z) {
    mm_zone_t *zone = NULL;

    if ((z < 0) || (z > (int)NELEM(zones)))
        return NULL;
    zone = &zones[z];
    mm_zone_lock(zone);
    // printk("requesting zone(%d): %p ,flags: %d\n", z, zone, zone->flags);
    if (!mm_zone_isvalid(zone)) {
        mm_zone_unlock(zone);
        return NULL;
    }
    return zone;
}

int mm_zone_contains(int z, uintptr_t addr, usize size) {
    mm_zone_t *zone = NULL;

    if ((z < MM_ZONE_DMA) || (z > MM_ZONE_HIGH))
        return -EINVAL;

    if (!(zone = get_mmzone(addr, size)))
        return -EINVAL;

    if (&zones[z] != zone) {
        mm_zone_unlock(zone);
        return -EINVAL;
    }

    mm_zone_unlock(zone);
    return 0;
}

int physical_memory_init(void) {
    int         err = 0;
    usize       index = 0;
    usize       size = 0;
    uintptr_t   addr = 0;
    mm_zone_t   *zone = NULL;
    page_t      *page = NULL;

    typeof(*bootinfo.mmap) *map = bootinfo.mmap;
    typeof(*bootinfo.mods) *module = bootinfo.mods;

    assert_msg(((bootinfo.memsize / 1024) > 64), "%s:%d: error: RAM = %dMiB : Not enough RAM!\n"
                                             "You need atleast 64MiB of RAM.\n",
           __FILE__, __LINE__, bootinfo.memsize / 1024);

    printk("initializing physical memory manager.\nRAM: %d KiB.\n", bootinfo.memsize);

    if ((err = enumerate_zones()))
        goto error;

    /**
     * Mark all memory maps returned by multiboot */
    for (usize i = 0; i < bootinfo.mmapcnt; ++i) {
        addr = VMA2LO(PGROUND(map[i].addr));
        size = PGROUNDUP(map[i].size);

        if (map[i].type != MULTIBOOT_MEMORY_AVAILABLE) {
            zone = get_mmzone(addr, size);
            if (zone != NULL) {
                index = (addr - zone->start) / PAGESZ;
                for (usize j = 0; j < (usize)NPAGE(size); ++j, ++index) {
                    page = &zone->pages[index];
                    atomic_write(&page->pg_refcnt, 1);
                    page_setmmzone(page, zone - zones);
                    page_setrwx(page);
                }

                zone->free_pages -= NPAGE(size);
                mm_zone_unlock(zone);
            }
        }
    }

    addr = PGROUND(VMA2LO(_kernel_start));
    size = PGROUNDUP((VMA2LO(_kernel_end) - VMA2LO(_kernel_start)));

    mm_zone_assert((zone = get_mmzone(addr, size)));
    assert_msg(((addr + size) < ((zone->size) - (addr + size))), "Kernel is too big");

    index = (addr - zone->start) / PAGESZ;
    for (usize j = 0; j < (usize)NPAGE(size); ++j, ++index) {
        page = &zone->pages[index];
        atomic_write(&page->pg_refcnt, 1);
        page_setmmzone(page, zone - zones);
        page_setrwx(page);
    }

    zone->free_pages -= NPAGE(size);
    mm_zone_unlock(zone);

    usize i = 0, j = 0;
    for (i = 0; i < bootinfo.modcnt; ++i) {
        size = PGROUNDUP(module[i].size);
        addr = PGROUND(VMA2LO(module[i].addr));
        for (j = 0; j < (usize)NPAGE(size); ++j, ++index, addr += PAGESZ) {
            zone = get_mmzone(addr, PAGESZ);
            zone->free_pages--;
            index   = (addr - zone->start) / PAGESZ;
            page    = &zone->pages[index];
            atomic_write(&page->pg_refcnt, 1);
            page_setmmzone(page, zone - zones);
            page_setrwx(page);
            mm_zone_unlock(zone);
        }
    }

    size = GiB(2) - PGROUNDUP(zones[MM_ZONE_NORM].size);

    if ((long)size > 0) {
        addr = PGROUNDUP(zones[MM_ZONE_NORM].size);
        arch_unmap_n(VMA2HI(addr), size);
    }

    for (usize i = 0; i < bootinfo.mmapcnt; ++i) {
        addr = PGROUND(map[i].addr);
        size = PGROUNDUP(map[i].size);

        if ((map[i].type != MULTIBOOT_MEMORY_AVAILABLE) && (VMA2LO(map[i].addr) < GiB(4))) {
            uint32_t flags = PTE_KRW | PTE_PCDWT;
            arch_map_i(addr, VMA2LO(addr), size, flags);
        }
    }

    arch_map_i(bootinfo.fb.framebuffer_addr, VMA2LO(bootinfo.fb.framebuffer_addr),
        bootinfo.fb.framebuffer_size, PTE_KRW | PTE_PCDWT);

    return 0;
error:
    return err;
}