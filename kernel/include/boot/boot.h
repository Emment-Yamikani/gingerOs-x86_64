#pragma once

#include <lib/types.h>
#include <lib/stddef.h>
#include <lib/stdint.h>
#include <boot/multiboot.h>
#include <dev/fb.h>

#define NMODS   32
#define NMMAP   32

typedef struct {
    uintptr_t   addr;
    size_t      size;
    char        *cmdline;
} mod_t;

typedef struct {
    uintptr_t   addr;
    size_t      size;
    int         type;
} mmap_t;

typedef struct {
    size_t      modcnt;
    size_t      memsize;
    size_t      memlo;
    size_t      memhigh;
    size_t      mmapcnt;

    struct {
        uint8_t framebuffer_type;
        uintptr_t framebuffer_addr;
        uint32_t framebuffer_pitch;
        uint32_t framebuffer_width;
        uint32_t framebuffer_height;
        uint32_t framebuffer_bpp;

        struct fb_bitfield red;
        struct fb_bitfield blue;
        struct fb_bitfield green;
        struct fb_bitfield resv;
    } fb;

    uintptr_t   phyaddr_start;

    mmap_t      mmap[NMMAP];
    mod_t       mods[NMODS];
} bootinfo_t;

extern bootinfo_t bootinfo;
int multiboot_info_process(multiboot_info_t *info);