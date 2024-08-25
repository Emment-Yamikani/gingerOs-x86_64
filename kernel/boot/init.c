#include <boot/boot.h>
#include <bits/errno.h>
#include <sys/system.h>
#include <lib/printk.h>
#include <lib/string.h>
#include <arch/x86_64/mmu.h>
#include <arch/x86_64/cpu.h>
#include <mm/pmm.h>
#include <arch/firmware/acpi.h>
#include <mm/vmm.h>
#include <arch/lapic.h>
#include <arch/paging.h>
#include <arch/chipset.h>
#include <sys/sched.h>
#include <sys/thread.h>
#include <dev/dev.h>
#include <sync/mutex.h>
#include <dev/fb.h>
#include <dev/console.h>
#include <mm/kalloc.h>
#include <arch/x86_64/ipi.h>

bootinfo_t bootinfo = {0};

extern __noreturn void kthread_main(void);

void multiboot_info_process(multiboot_info_t *mbi) {
    // Initialize bootinfo with zero
    memset(&bootinfo, 0, sizeof(bootinfo_t));
    

    // Get memory information
    bootinfo.memlo      = (usize)mbi->mem_lower;
    bootinfo.memhigh    = (usize)mbi->mem_upper;

    // Get memory map information
    if (mbi->flags & MULTIBOOT_INFO_MEM_MAP) {
        boot_mmap_t     *mmap   = bootinfo.mmap;
        mmap_entry_t    *entry  = (mmap_entry_t *)VMA2HI(mbi->mmap_addr);
        mmap_entry_t    *end    = (mmap_entry_t *)VMA2HI(mbi->mmap_addr + mbi->mmap_length);

        for (; entry < end; mmap++, bootinfo.mmapcnt++) {
            if (bootinfo.mmapcnt >= NMMAP) break;
            mmap->type        = entry->type;
            mmap->addr        = VMA2HI(entry->addr);
            mmap->size        = entry->len;
            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
                bootinfo.memsize += entry->len;
            entry = (mmap_entry_t *)((u64)entry + entry->size + sizeof(entry->size));
        }
        
        bootinfo.memsize = PGROUND(bootinfo.memsize) / KiB(1);
    }

    bootinfo.phyaddr = PGROUNDUP(_kernel_end);
    
    // Get modules information
    if (mbi->flags & MULTIBOOT_INFO_MODS) {
        mod_entry_t *mod = (mod_entry_t *)VMA2HI(mbi->mods_addr);
        bootinfo.modcnt = mbi->mods_count > NMODS ? NMODS : mbi->mods_count;
        for (usize i = 0; i < bootinfo.modcnt; i++) {
            bootinfo.mods[i].addr    = VMA2HI(mod[i].mod_start);
            bootinfo.mods[i].cmdline = (char *)(VMA2HI(mod[i].cmdline));
            bootinfo.mods[i].size    = PGROUNDUP(mod[i].mod_end - mod[i].mod_start);
            bootinfo.phyaddr         = VMA2LO(bootinfo.mods[i].addr) + bootinfo.mods[i].size;
        }
    }

    // Get framebuffer information
    if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        bootinfo.fb.bpp     = mbi->framebuffer_bpp;
        bootinfo.fb.type    = mbi->framebuffer_type;
        bootinfo.fb.pitch   = mbi->framebuffer_pitch;
        bootinfo.fb.width   = mbi->framebuffer_width;
        bootinfo.fb.height  = mbi->framebuffer_height;
        bootinfo.fb.addr    = VMA2HI(mbi->framebuffer_addr);
        bootinfo.fb.size    = mbi->framebuffer_height * mbi->framebuffer_pitch;
    }

    // Get bootloader name
    if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
        // Store or process the bootloader name if needed
    }

    usize size = GiB(2) - (bootinfo.memsize * KiB(1));
    arch_unmap_n(VMA2HI(bootinfo.memsize * KiB(1)), size);
}

#include <dev/cga.h>

int early_init(void) {
    int err = 0;

    if ((err = bsp_init()))
        panic("BSP initialization failed, error: %d\n", err);

    if ((err = vmman.init()))
        panic("Virtual memory initialization failed, error: %d\n", err);

    if ((err = pmman.init()))
        panic("Physical memory initialization failed, error: %d\n", err);

    earlycons_usefb();

    if ((err = acpi_init()))
        panic("Failed to initialize ACPI, error: %d\n", err);

    bootothers();

    pic_init();
    ioapic_init();

    if ((err = dev_init()))
        panic("Failed to start devices, error: %d\n", err);

    if ((err = vfs_init()))
        panic("Failed to initialize VFS!, error: %d\n", err);

    kthread_create(
        NULL, (thread_entry_t)kthread_main,
        NULL, THREAD_CREATE_GROUP |
        THREAD_CREATE_SCHED, NULL
    );

    schedule();
    assert(0, "scheduler returned :(");
    loop();
    return 0;
}