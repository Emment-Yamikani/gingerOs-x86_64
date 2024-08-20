#pragma once

#include <boot/multiboot.h>
#include <core/types.h>

#define NMODS   32
#define NMMAP   32

typedef struct {
    uintptr_t   addr;
    usize      size;
    char        *cmdline;
} mod_t;

typedef struct {
    uintptr_t   addr;
    usize      size;
    int         type;
} multiboot_mmap_t;

typedef struct {
    usize      modcnt;
    usize      memsize;
    usize      memlo;
    usize      memhigh;
    usize      mmapcnt;

    struct {
        u8 framebuffer_type;
        uintptr_t framebuffer_addr;
        u32 framebuffer_pitch;
        u32 framebuffer_width;
        u32 framebuffer_height;
        usize   framebuffer_size;
        u32 framebuffer_bpp;

        // struct fb_bitfield red;
        // struct fb_bitfield blue;
        // struct fb_bitfield green;
        // struct fb_bitfield resv;
    } fb;

    uintptr_t   phyaddr_start;

    multiboot_mmap_t      mmap[NMMAP];
    mod_t       mods[NMODS];
} bootinfo_t;

typedef multiboot_module_t mod_entry_t;
typedef multiboot_memory_map_t mmap_entry_t;

extern bootinfo_t bootinfo;
int multiboot_info_process(multiboot_info_t *info);