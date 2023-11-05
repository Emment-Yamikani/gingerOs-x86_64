#include <mm/mm_zone.h>
#include <mm/page.h>
#include <bits/errno.h>
#include <sys/system.h>
#include <lib/printk.h>
#include <mm/pmm.h>
#include <boot/boot.h>
#include <arch/x86_64/pml4.h>
#include <lib/string.h>

mm_zone_t zones[NZONE] = {0};
const char *str_zone[] = {
    "DMA",
    "NORM",
    "HOLE",
    "HIGH",
    NULL,
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
           zone->start, zone->size, zone->size / MiB, zone->free_pages, zone->nrpages, zone->pages);
}

static int enumerate_zones(void) {
    void *pages = NULL;
    uintptr_t start = 0x0;
    size_t size = (16 * MiB);
    size_t zone_structs_size = 0;
    size_t memsize = bootinfo.memsize * KiB;
    mod_t *last_mod = &bootinfo.mods[bootinfo.modcnt - 1];
    pages = (void *)PGROUNDUP((last_mod->addr + last_mod->size));
    zone_structs_size = NPAGE(size) * sizeof(page_t);

    printk("enumerating DMA memory zone...\n");

    zones[MM_ZONE_DMA] = (mm_zone_t){
        .pages = pages,
        .nrpages = size / PGSZ,
        .flags = MM_ZONE_VALID,
        .lock = SPINLOCK_INIT(),
        .start = PGROUNDUP(start),
        .free_pages = size / PGSZ,
        .size = size,
    };

    start = (16 * MiB);
    memsize -= (16 * MiB);
    size = (2 * GiB) - (16 * MiB);
    size = MIN(memsize, size);
    memsize -= size;
    zone_structs_size = NPAGE(size) * sizeof(page_t);
    pages += NPAGE(size) * sizeof(page_t);

    printk("enumerating NORM memory zone...\n");

    zones[MM_ZONE_NORM] = (mm_zone_t){
        .pages = pages,
        .nrpages = size / PGSZ,
        .flags = MM_ZONE_VALID,
        .lock = SPINLOCK_INIT(),
        .start = PGROUNDUP(start),
        .free_pages = size / PGSZ,
        .size = size,
    };

    if (!memsize || !bootinfo.memhigh)
        goto done;

    printk("enumerating HOLE memory zone...\n");

    start = 2 * GiB;
    size = ((bootinfo.memhigh * KiB) + MiB) - ((2 * GiB));
    size = MIN(memsize, size);
    memsize -= size;
    zone_structs_size = NPAGE(size) * sizeof(page_t);
    pages += NPAGE(size) * sizeof(page_t);


    zones[MM_ZONE_HOLE] = (mm_zone_t){
        .pages = pages,
        .nrpages = size / PGSZ,
        .flags = MM_ZONE_VALID,
        .lock = SPINLOCK_INIT(),
        .start = PGROUNDUP(start),
        .free_pages = size / PGSZ,
        .size = size,
    };

    if (!memsize)
        goto done;

    printk("enumerating HIGH memory zone...\n");

    start = 4 * GiB;
    size = memsize;
    zone_structs_size = NPAGE(size) * sizeof(page_t);
    pages += NPAGE(size) * sizeof(page_t);

    zones[MM_ZONE_HIGH] = (mm_zone_t){
        .pages = pages,
        .nrpages = size / PGSZ,
        .flags = MM_ZONE_VALID,
        .lock = SPINLOCK_INIT(),
        .start = PGROUNDUP(start),
        .free_pages = size / PGSZ,
        .size = size,
    };

done:
    pages = zones[MM_ZONE_DMA].pages;
    memset(pages, 0, zone_structs_size);
    return 0;
}

mm_zone_t *get_mmzone(uintptr_t addr, size_t size) {
    for (size_t i = 0; i < NELEM(zones); ++i) {
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
    if (!mm_zone_isvalid(zone))
    {
        mm_zone_unlock(zone);
        return NULL;
    }
    return zone;
}

int mm_zone_contains(int z, uintptr_t addr, size_t size) {
    mm_zone_t *zone = NULL;

    if ((z < MM_ZONE_DMA) || (z > MM_ZONE_HIGH))
        return -EINVAL;

    if (!(zone = get_mmzone(addr, size)))
        return -EINVAL;

    if (&zones[z] != zone)
    {
        mm_zone_unlock(zone);
        return -EINVAL;
    }

    mm_zone_unlock(zone);
    return 0;
}

int physical_memory_init(void) {
    int err = 0;
    size_t index = 0;
    size_t size = 0;
    uintptr_t addr = 0;
    mm_zone_t *zone = NULL;
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
    for (size_t i = 0; i < bootinfo.mmapcnt; ++i) {
        addr = PGROUND(map[i].addr);
        size = PGROUNDUP(map[i].size);

        if (map[i].type != MULTIBOOT_MEMORY_AVAILABLE) {
            zone = get_mmzone(addr, size);
            if (zone) {
                index = (addr - zone->start) / PAGESZ;
                for (size_t j = 0; j < (size_t)NPAGE(size); ++j, ++index)
                {
                    atomic_write(&zone->pages[index].ref_count, 1);
                    zone->pages[index].flags.mm_zone = zone - zones;
                    zone->pages[index].flags.raw |= VM_KRW;
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
    for (size_t j = 0; j < (size_t)NPAGE(size); ++j, ++index) {
        atomic_write(&zone->pages[index].ref_count, 1);
        zone->pages[index].flags.mm_zone = zone - zones;
        zone->pages[index].flags.raw |= VM_KRW;
    }

    zone->free_pages -= NPAGE(size);
    mm_zone_unlock(zone);

    size_t i = 0, j = 0;
    for (i = 0; i < bootinfo.modcnt; ++i) {
        size = PGROUNDUP(module[i].size);
        addr = PGROUND(VMA2LO(module[i].addr));
        for (j = 0; j < (size_t)NPAGE(size); ++j, ++index, addr += PAGESZ) {
            zone = get_mmzone(addr, PAGESZ);
            zone->free_pages--;
            index = (addr - zone->start) / PAGESZ;
            zone->pages[index].ref_count = 1;
            zone->pages[index].flags.mm_zone = zone - zones;
            zone->pages[index].flags.raw |= VM_KRW;
            mm_zone_unlock(zone);
        }
    }

    size = zones[MM_ZONE_NORM].size - ((1 * GiB) - (16 * MiB));
    size = ((2 * GiB)) - PGROUNDUP(zones[MM_ZONE_NORM].size);

    pagemap_binary_lock(&kernel_map);
    if ((long)size > 0) {
        addr = PGROUNDUP(zones[MM_ZONE_NORM].size);
        if ((err = x86_64_unmap_n(kernel_map.pdbr, VMA2HI(addr), size, 0)))
            panic("failed to unmap: %lX, err: %d\n", addr, err);
    }

    for (size_t i = 0; i < bootinfo.mmapcnt; ++i) {
        addr = PGROUND(map[i].addr);
        size = PGROUNDUP(map[i].size);

        if ((map[i].type != MULTIBOOT_MEMORY_AVAILABLE) && map[i].addr < 0x100000000) {
            uint32_t flags = VM_KRW | VM_PCDWT;
            x86_64_map_to_n(kernel_map.pdbr, VMA2HI(addr), addr, size, flags);
        }
    }

    // unmap_table_entry(&kernel_map, LVL_PML4E, 0, 0, 0, 0);
    // unmap_table_entry(&kernel_map, LVL_PDPTE, PML4I(VMA2HI(0xC0000000)), PDPTI(0xC0000000), 0, 0);
    x86_64_map_to_n(kernel_map.pdbr, VMA2HI(MEMMDEV), MEMMDEV, (4 * GiB) - MEMMDEV, VM_KRW | VM_PCD);
    pagemap_binary_unlock(&kernel_map);
    return 0;
error:
    return err;
}