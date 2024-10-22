#pragma once


#include <boot/multiboot.h>
#include <lib/types.h>
#include <dev/fb.h>

#define NMODS   32
#define NMMAP   32

typedef struct {
    uintptr_t   addr;   // address at which module is loaded.
    usize       size;   // size of the module.
    char        *cmd;   // command line passed with the module
} mod_t;

typedef struct {
    uintptr_t   addr;   // address at which the memory map starts.
    usize       size;   // size of the memory map.
    int         type;   // type of memory this mmap describes.
} boot_mmap_t;

typedef struct {
    usize       total;      // total available memory.
    usize       usable;     // Size of usable physical memory.
    usize       memlo;      // Size of lower memory.
    usize       memhi;      // Size of Higher memory.

    struct {
        u8          type;
        uintptr_t   addr;
        u32         pitch;
        u32         width;
        u32         height;
        usize       size;
        u32         bpp;

        struct fb_bitfield red;
        struct fb_bitfield blue;
        struct fb_bitfield green;
        struct fb_bitfield resv;
    } fb /* framebuffer data returned by bootloader*/;

    uintptr_t       phyaddr;    // first free physical address. 
    boot_mmap_t     mmap[NMMAP];// array of memory maps.
    u32             mmapcnt;    // # of memory maps
    
    mod_t           mods[NMODS];// array of available modules.
    u32             modcnt;     // # of modules
    
    uintptr_t       kern_base;
    usize           kern_size;
} bootinfo_t;

typedef multiboot_module_t mod_entry_t;
typedef multiboot_memory_map_t mmap_entry_t;

extern bootinfo_t bootinfo;

extern void boot_mmap_dump(void);