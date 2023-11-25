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

int multiboot_info_process(multiboot_info_t *info) {
    mmap_entry_t *mmap = NULL, *mmap_end = NULL;

    memset(&bootinfo, 0, sizeof bootinfo);

    if (BTEST(info->flags, 0)) {
        bootinfo.memlo = info->mem_lower;
        bootinfo.memhigh = info->mem_upper;
    }

    if ((BTEST(info->flags, 6))) {
        mmap = (mmap_entry_t *)(VMA2HI(info->mmap_addr));
        mmap_end = (mmap_entry_t *)VMA2HI(info->mmap_addr + info->mmap_length);

        for (int i =0; mmap < mmap_end; ++i, bootinfo.mmapcnt++) {
            bootinfo.mmap[i].size = mmap->len;
            bootinfo.mmap[i].type = mmap->type;
            bootinfo.mmap[i].addr = VMA2HI(mmap->addr);
            if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
                bootinfo.memsize      += mmap->len;
            // printk("mmap(%d): %p, len: %ld, type: %d, size: %d\n",
                // i, VMA2HI(mmap->addr), mmap->len, mmap->type, mmap->size);
            mmap = (mmap_entry_t *)(((uintptr_t)mmap) + mmap->size + sizeof (mmap->size));
        }
        bootinfo.memsize /= 1024;
    }
    else
        return -ENOMEM;

    if (BTEST(info->flags, 3)) {
        multiboot_module_t *mod = (multiboot_module_t *)(VMA2HI(info->mods_addr));
        for (size_t i = 0; i < info->mods_count; ++i, bootinfo.modcnt++, ++mod) {
            bootinfo.mods[i].addr = VMA2HI(mod->mod_start);
            bootinfo.mods[i].cmdline = (char *)VMA2HI(mod->cmdline);
            bootinfo.mods[i].size = mod->mod_end - mod->mod_start;
            // printk("MOD(%d): %p, size: %d\n", i, mod->mod_start, mod->mod_end - mod->mod_start);
        }
    }

    // framebuffer
    if (BTEST(info->flags, 12)) {
        bootinfo.fb.framebuffer_bpp = info->framebuffer_bpp;
        bootinfo.fb.framebuffer_type = info->framebuffer_type;
        bootinfo.fb.framebuffer_pitch = info->framebuffer_pitch;
        bootinfo.fb.framebuffer_width = info->framebuffer_width;
        bootinfo.fb.framebuffer_height = info->framebuffer_height;
        bootinfo.fb.framebuffer_addr = VMA2HI(info->framebuffer_addr);
        bootinfo.fb.framebuffer_size = info->framebuffer_pitch * info->framebuffer_height;

        if (info->framebuffer_type == 1) {
            bootinfo.fb.red.length = info->framebuffer_red_mask_size;
            bootinfo.fb.red.offset = info->framebuffer_red_field_position;
            bootinfo.fb.green.length = info->framebuffer_green_mask_size;
            bootinfo.fb.green.offset = info->framebuffer_green_field_position;
            bootinfo.fb.blue.length = info->framebuffer_blue_mask_size;
            bootinfo.fb.blue.offset = info->framebuffer_blue_field_position;
        }
    }

    return 0;
}

extern __noreturn void kthread_main(void);

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

    thread_create(NULL, NULL, (thread_entry_t)kthread_main, NULL);

    schedule();
    assert(0, "scheduler returned :(");
    loop();
    return 0;
}