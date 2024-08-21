#pragma once

#include <boot/multiboot.h>
#include <core/types.h>

#define NMODS   32
#define NMMAP   32

typedef struct {
    uintptr_t   addr;
    usize       size;
    char        *cmdline;
} mod_t;

typedef struct {
    uintptr_t   addr;
    usize       size;
    int         type;
} boot_mmap_t;

typedef struct {
    usize           modcnt;
    usize           memsize;
    usize           memlo;
    usize           memhigh;
    usize           mmapcnt;

    uintptr_t       phyaddr;
    mod_t           mods[NMODS];
    boot_mmap_t     mmap[NMMAP];

    struct {
        u8          type;
        uintptr_t   addr;
        u32         pitch;
        u32         width;
        u32         height;
        usize       size;
        u32         bpp;

        // struct fb_bitfield red;
        // struct fb_bitfield blue;
        // struct fb_bitfield green;
        // struct fb_bitfield resv;
    } fb;
} bootinfo_t;

typedef multiboot_module_t mod_entry_t;
typedef multiboot_memory_map_t mmap_entry_t;

extern bootinfo_t bootinfo;
int multiboot_info_process(multiboot_info_t *info);