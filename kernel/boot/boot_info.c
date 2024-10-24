#include <boot/boot.h>
#include <sys/system.h>
#include <lib/string.h>
#include <mm/page.h>
#include <lib/printk.h>

bootinfo_t bootinfo = {0};

void boot_mmap_dump(void) {
    for (u32 i = 0; i < bootinfo.mmapcnt; ++i) {
        printk("mmap(%d): addr: %p, type: %d, size: %10d Bytes\n",
            i,
            bootinfo.mmap[i].addr,
            bootinfo.mmap[i].type,
            bootinfo.mmap[i].size
        );
    }

    printk("memory info:\n"
            "Total:  %d kB\n"
            "Usable: %d KB\n",
            bootinfo.total,
            bootinfo.usable
    );
}

// Swap two boot_mmap_t elements
void swap_mmap(boot_mmap_t *a, boot_mmap_t *b) {
    boot_mmap_t temp;

    temp.addr = a->addr;
    temp.size = a->size;
    temp.type = a->type;

    a->addr = b->addr;
    a->size = b->size;
    a->type = b->type;

    b->addr = temp.addr;
    b->size = temp.size;
    b->type = temp.type;
}

// Bubble Sort function to sort the memory map array based on addr
void bubble_sort_mmap(boot_mmap_t *array, size_t count) {
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - 1 - i; j++) {
            if (array[j].addr > array[j + 1].addr) {
                // Swap elements if they are in the wrong order
                swap_mmap(&array[j], &array[j + 1]);
            }
        }
    }
}

void multiboot_info_process(multiboot_info_t *mbi) {
    boot_mmap_t     *mmap   = bootinfo.mmap;
    
    // Initialize bootinfo with zero
    memset(&bootinfo, 0, sizeof(bootinfo_t));

    // Setup kernel address and size.
    bootinfo.kern_base  = PGROUND(kstart);
    bootinfo.kern_size  = PGROUNDUP(kend - kstart);

    // Get memory information
    bootinfo.memlo      = (usize)mbi->mem_lower;
    bootinfo.memhi      = (usize)mbi->mem_upper;
    bootinfo.total      = M2KiB(1) + mbi->mem_upper;

    // Get memory map information
    if (mbi->flags & MULTIBOOT_INFO_MEM_MAP) {
        mmap_entry_t    *entry  = (mmap_entry_t *)V2HI(mbi->mmap_addr);
        mmap_entry_t    *end    = (mmap_entry_t *)V2HI(mbi->mmap_addr + mbi->mmap_length);

        /** Consider the memory region
         * occupied by the kernel as reversed.*/
        bootinfo.mmapcnt = 1;
        mmap->addr       = bootinfo.kern_base;
        mmap->size       = bootinfo.kern_size;
        mmap->type       = MULTIBOOT_MEMORY_RESERVED;
        mmap += 1;  // move the next mmap slot.

        for ( ; entry < end; mmap++) {
            // highly unlikely, but just to be on a safe size ;).
            if (bootinfo.mmapcnt >= NMMAP) break;

            // only consider memory within 32bit address space.
            if (entry->addr < GiB(4)) {
                mmap->type          = entry->type;
                mmap->addr          = V2HI(entry->addr);
                mmap->size          = entry->len;

                /* FIXME: 
                sneaky but it works add to the total the size of
                ACPI-reclaimable memory which according to my observation
                is located at (memhi + 1Mib) which coinsides with first
                memory hole mentioned by the multiboot doumentation.*/
                if (KiB(bootinfo.total) == entry->addr) {
                    bootinfo.total += B2KiB(mmap->size);
                }

                if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                    bootinfo.usable += (usize)entry->len;
                bootinfo.mmapcnt++;
            }
            entry = (mmap_entry_t *)((u64)entry + entry->size + sizeof(entry->size));
        }
    }

    // bootinfo.total   = bootinfo.total / KiB(1); 
    /// the first free physical address
    /// is the page frame right after the kernel.
    bootinfo.phyaddr = PGROUNDUP(kend);
    
    // Get modules information
    if (mbi->flags & MULTIBOOT_INFO_MODS) {
        mod_entry_t *mod = (mod_entry_t *)V2HI(mbi->mods_addr);
        bootinfo.modcnt = mbi->mods_count > NMODS ? NMODS : mbi->mods_count;
        for (usize i = 0; i < bootinfo.modcnt; i++) {
            bootinfo.mods[i].addr    = V2HI(mod[i].mod_start);
            bootinfo.mods[i].cmd     = (char *)(V2HI(mod[i].cmdline));
            bootinfo.mods[i].size    = PGROUNDUP(mod[i].mod_end - mod[i].mod_start);
            bootinfo.phyaddr         = V2LO(bootinfo.mods[i].addr) + bootinfo.mods[i].size;
        }
    }

    /** Add the region holding the page_t array as a reserved region. */
    mmap = &bootinfo.mmap[bootinfo.mmapcnt];
    mmap->addr = V2HI(bootinfo.phyaddr);
    mmap->size = (bootinfo.total / 4) * sizeof(page_t); // divide by 4 because already in Kib.
    mmap->type = MULTIBOOT_MEMORY_RESERVED;
    bootinfo.mmapcnt++;
    
    /// subtract to account for space used
    /// by kernel and by the array of page_t.
    bootinfo.usable -= mmap->size + bootinfo.kern_size;
    // memory sizes must be in KiB for both usable and total memory.
    bootinfo.usable  = PGROUND(bootinfo.usable) / KiB(1);

    // sort mmaps' array.
    bubble_sort_mmap(bootinfo.mmap, bootinfo.mmapcnt);

    // Get framebuffer information
    if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        bootinfo.fb.bpp     = mbi->framebuffer_bpp;
        bootinfo.fb.type    = mbi->framebuffer_type;
        bootinfo.fb.pitch   = mbi->framebuffer_pitch;
        bootinfo.fb.width   = mbi->framebuffer_width;
        bootinfo.fb.height  = mbi->framebuffer_height;
        bootinfo.fb.addr    = V2HI(mbi->framebuffer_addr);
        bootinfo.fb.size    = mbi->framebuffer_height * mbi->framebuffer_pitch;
    }

    // Get bootloader name
    if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
        // Store or process the bootloader name if needed
    }
}