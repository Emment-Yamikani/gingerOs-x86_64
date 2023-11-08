#include <dev/dev.h>
#include <lib/string.h>
#include <sync/spinlock.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <dev/fb.h>
#include <boot/boot.h>
#include <dev/console.h>

static struct dev fbdev = {0};
fb_fixinfo_t fix_info = {0};
fb_varinfo_t var_info = {0};
framebuffer_t fbs[NFBDEV];

int framebuffer_gfx_init(void) {
    if (bootinfo.fb.framebuffer_type != 1)
        return -ENOENT;

    memset(fbs, 0, sizeof fbs);
    memset(&fix_info, 0, sizeof fix_info);
    memset(&var_info, 0, sizeof var_info);

    fix_info.addr = bootinfo.fb.framebuffer_addr;
    fix_info.memsz = bootinfo.fb.framebuffer_pitch * bootinfo.fb.framebuffer_height;
    fix_info.id[0] = 'V';
    fix_info.id[1] = 'E';
    fix_info.id[2] = 'S';
    fix_info.id[3] = 'A';
    fix_info.id[4] = '_';
    fix_info.id[5] = 'V';
    fix_info.id[6] = 'B';
    fix_info.id[7] = 'E';
    fix_info.id[8] = '3';
    fix_info.line_length = bootinfo.fb.framebuffer_pitch;
    fix_info.type = bootinfo.fb.framebuffer_type;

    var_info.colorspace = 1;
    var_info.bpp = bootinfo.fb.framebuffer_bpp;
    var_info.pitch = bootinfo.fb.framebuffer_pitch;
    var_info.width = bootinfo.fb.framebuffer_width;
    var_info.height = bootinfo.fb.framebuffer_height;
    var_info.red = bootinfo.fb.red;
    var_info.blue = bootinfo.fb.blue;
    var_info.green = bootinfo.fb.green;
    var_info.transp = bootinfo.fb.resv;

    fbs[0].dev = &fbdev;
    fbs[0].priv = &bootinfo.fb;
    fbs[0].module = 0;
    fbs[0].fixinfo = &fix_info;
    fbs[0].varinfo = &var_info;
    earlycons_use_gfx = 1;
    return 0;
}