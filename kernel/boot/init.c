#include <boot/boot.h>
#include <boot/multiboot.h>
#include <core/core.h>
#include <sys/thread.h>

bootinfo_t bootinfo = {0};

void parse_multiboot_info(multiboot_info_t *mbi) {
    // Initialize bootinfo with zero
    bzero(&bootinfo, sizeof(bootinfo_t));
    
    // Get memory information
    bootinfo.memlo      = (usize)mbi->mem_lower;
    bootinfo.memhigh    = (usize)mbi->mem_upper;
    bootinfo.memsize    = (usize)(mbi->mem_upper + 1024) * 1024;  // Convert to bytes
    
    // Get modules information
    if (mbi->flags & MULTIBOOT_INFO_MODS) {
        multiboot_module_t *mod = (multiboot_module_t *)((uintptr_t)mbi->mods_addr);
        bootinfo.modcnt = mbi->mods_count > NMODS ? NMODS : mbi->mods_count;
        
        for (usize i = 0; i < bootinfo.modcnt; i++) {
            bootinfo.mods[i].cmdline = (char *)((uintptr_t)mod[i].cmdline);
            bootinfo.mods[i].addr    = (uintptr_t)mod[i].mod_start;
            bootinfo.mods[i].size    = (usize)mod[i].mod_end - mod[i].mod_start;
        }
    }
    
    // Get memory map information
    if (mbi->flags & MULTIBOOT_INFO_MEM_MAP) {
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)((uintptr_t)mbi->mmap_addr);
        while ((unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length) {
            if (bootinfo.mmapcnt >= NMMAP) break;
            bootinfo.mmap[bootinfo.mmapcnt].addr = mmap->addr;
            bootinfo.mmap[bootinfo.mmapcnt].size = mmap->len;
            bootinfo.mmap[bootinfo.mmapcnt].type = mmap->type;
            mmap = (multiboot_memory_map_t *)((unsigned long)mmap + mmap->size + sizeof(mmap->size));
            bootinfo.mmapcnt++;
        }
    }

    // Get framebuffer information
    if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) {
        bootinfo.fb.type    = mbi->framebuffer_type;
        bootinfo.fb.addr    = mbi->framebuffer_addr;
        bootinfo.fb.pitch   = mbi->framebuffer_pitch;
        bootinfo.fb.width   = mbi->framebuffer_width;
        bootinfo.fb.height  = mbi->framebuffer_height;
        bootinfo.fb.bpp     = mbi->framebuffer_bpp;
        bootinfo.fb.size    = mbi->framebuffer_height * mbi->framebuffer_pitch;
    }

    // Get bootloader name
    if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
        // Store or process the bootloader name if needed
    }

    // Store physical address of Multiboot info structure for reference
    bootinfo.phyaddr = (uintptr_t)mbi;
}

void early_init(void) {

    loop();
}