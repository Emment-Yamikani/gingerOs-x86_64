#include <dev/dev.h>
#include <lib/string.h>
#include <sync/spinlock.h>
#include <bits/errno.h>
#include <mm/kalloc.h>
#include <dev/fb.h>
#include <boot/boot.h>
#include <dev/console.h>
#include <mm/mmap.h>
#include <arch/paging.h>

static int fbdev_init(void);
static int fbdev_probe(void);
static int fbdev_close(struct devid *dd);
static int fbdev_getinfo(struct devid *dd, void *info);
static int fbdev_open(struct devid *dd, int oflags, ...);
static int fbdev_ioctl(struct devid *dd, int req, void *argp);
static off_t fbdev_lseek(struct devid *dd, off_t off, int whence);
static ssize_t fbdev_read(struct devid *dd, off_t off, void *buf, size_t sz);
static ssize_t fbdev_write(struct devid *dd, off_t off, void *buf, size_t sz);

int fbdev_vmr_fault(vmr_t *region, vm_fault_t *fault);
static int fbdev_mmap(struct devid *dd, vmr_t *region);

static dev_t fbdev;
fb_fixinfo_t fix_info       = {0};
fb_varinfo_t var_info       = {0};
framebuffer_t fbs[NFBDEV]   = {0};

static vmr_ops_t fbdev_vmrops = {
    .io = NULL,
    .fault = fbdev_vmr_fault,
};

#define fblock(fb)      ({ spin_lock(&(fb)->lock); })
#define fbunlock(fb)    ({ spin_unlock(&(fb)->lock); })
#define fbislocked(fb)  ({ spin_islocked(&(fb)->lock); })


static DEV_INIT(fbdev, FS_CHR, DEV_FB, 0);

int framebuffer_gfx_init(void) {
    if (bootinfo.fb.framebuffer_type != 1)
        return -ENOENT;

    memset(fbs, 0, sizeof fbs);
    memset(&fix_info, 0, sizeof fix_info);
    memset(&var_info, 0, sizeof var_info);

    fix_info.addr       = bootinfo.fb.framebuffer_addr;
    fix_info.memsz      = bootinfo.fb.framebuffer_pitch * bootinfo.fb.framebuffer_height;
    fix_info.id[0]      = 'V';
    fix_info.id[1]      = 'E';
    fix_info.id[2]      = 'S';
    fix_info.id[3]      = 'A';
    fix_info.id[4]      = '_';
    fix_info.id[5]      = 'V';
    fix_info.id[6]      = 'B';
    fix_info.id[7]      = 'E';
    fix_info.id[8]      = '3';
    fix_info.line_length= bootinfo.fb.framebuffer_pitch;
    fix_info.type       = bootinfo.fb.framebuffer_type;

    var_info.colorspace = 1;
    var_info.red        = bootinfo.fb.red;
    var_info.transp     = bootinfo.fb.resv;
    var_info.blue       = bootinfo.fb.blue;
    var_info.green      = bootinfo.fb.green;
    var_info.bpp        = bootinfo.fb.framebuffer_bpp;
    var_info.pitch      = bootinfo.fb.framebuffer_pitch;
    var_info.width      = bootinfo.fb.framebuffer_width;
    var_info.height     = bootinfo.fb.framebuffer_height;

    fbs[0].dev          = &fbdev;
    fbs[0].priv         = &bootinfo.fb;
    fbs[0].module       = 0;
    fbs[0].fixinfo      = &fix_info;
    fbs[0].varinfo      = &var_info;
    earlycons_use_gfx   = 1;
    return 0;
}

static int fbdev_init(void) {
    return 0;
}

static int fbdev_probe(void) {
    return 0;
}

static int fbdev_close(struct devid *dd) {
    if (dd == NULL ||
        dd->major != DEV_FB ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;
    
    return 0;
}

static int fbdev_getinfo(struct devid *dd, void *info __unused) {
    if (dd == NULL ||
        dd->major != DEV_FB ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;
    
    return 0;
}

static int fbdev_open(struct devid *dd, int oflags __unused, ...) {
    if (dd == NULL ||
        dd->major != DEV_FB ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;
    return 0;
}

static int fbdev_ioctl(struct devid *dd, int req, void *argp) {
    int err = 0;
    framebuffer_t *fb = NULL;

    if (dd == NULL ||
        dd->major != DEV_FB ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;

    /// TODO: may need an explicit locking mechanism here.
    /// To protect fbs[] array.
    fb = &fbs[dd->minor];
    
    fblock(fb);

    switch (req) {
    case FBIOGET_FIX_INFO:
        if (fb->fixinfo == NULL) {
            err = -EINVAL;
            break;
        }

        memcpy(argp, fb->fixinfo, sizeof *fb->fixinfo);
        break;
    case FBIOGET_VAR_INFO:
        if (fb->varinfo == NULL) {
            err = -EINVAL;
            break;
        }

        memcpy(argp, fb->varinfo, sizeof *fb->varinfo);
        break;
    default:
        err = -EINVAL;
    }

    fbunlock(fb);

    return err;
}

static off_t fbdev_lseek(struct devid *dd, off_t off, int whence) {
    if (dd == NULL ||
        dd->major != DEV_FB ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;
    
    return -EINVAL;
}

static ssize_t fbdev_read(struct devid *dd, off_t off, void *buf, size_t sz) {
    int             err  = 0;
    ssize_t         size = 0;
    framebuffer_t   *fb  = NULL;

    if (dd == NULL ||
        dd->major != DEV_FB ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;

    /// TODO: may need an explicit locking mechanism here.
    /// To protect fbs[] array.
    fb = &fbs[dd->minor];
    

    // TODO: Do i need to lock fb???

    if (fb->fixinfo == NULL)
        return -EINVAL;

    if (off > fb->fixinfo->memsz)
        return -ERANGE; // Out of range not allowed.

    size = MIN(sz, fb->fixinfo->memsz - off);
    size = (size_t)memcpy(buf, (void *)(fb->fixinfo->addr + off), size) - 
        (size_t)buf;

    return size;
}

static ssize_t fbdev_write(struct devid *dd, off_t off, void *buf, size_t sz) {
    int             err  = 0;
    ssize_t         size = 0;
    framebuffer_t   *fb  = NULL;

    if (dd == NULL ||
        dd->major != DEV_FB ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;

    /// TODO: may need an explicit locking mechanism here.
    /// To protect fbs[] array.
    fb = &fbs[dd->minor];
    

    // TODO: Do i need to lock fb???

    if (fb->fixinfo == NULL)
        return -EINVAL;

    if (off > fb->fixinfo->memsz)
        return -ERANGE; // Out of range not allowed.

    size = MIN(sz, fb->fixinfo->memsz - off);
    size = (size_t)memcpy((void *)(fb->fixinfo->addr + off), buf, size) - 
        (fb->fixinfo->addr + off);

    return size;
}

int fbdev_vmr_fault(vmr_t *region, vm_fault_t *fault) {
    uintptr_t   fbaddr = 0;
    framebuffer_t *fb  = NULL;

    if (region == NULL || fault == NULL)
        return -EINVAL;

    if (__vmr_exec(region) || __vmr_dontexpand(region))
        return -EINVAL;

    if (!__vmr_read(region) && !__vmr_write(region))
        return -EACCES;
    
    /**
     * @brief In a twist of fate, region->priv has a purpose.
     * I never imagened it would workout this way! ;).
     */
    fb = (framebuffer_t *)region->priv;

    // TODO: Do i need to lock fb here.

    if (fb->fixinfo == NULL)
        return -EINVAL;

    if (__vmr_filepos(region) > fb->fixinfo->memsz)
        return -ERANGE; // Out of range not allowed.

    fbaddr = PGROUND(fb->fixinfo->addr + __vmr_filepos(region));
    return arch_map_i(fault->addr, fbaddr, PGSZ, region->vflags);
}

static int fbdev_mmap(struct devid *dd, vmr_t *region) {
    int err = 0;
    framebuffer_t *fb = NULL;

    if (dd == NULL ||
        dd->major != DEV_FB ||
        dd->minor >= NFBDEV || dd->type != FS_CHR)
        return -EINVAL;

    if (region == NULL)
        return -EINVAL;

    /// TODO: may need an explicit locking mechanism here.
    /// To protect fbs[] array.
    fb = &fbs[dd->minor];
    
    if (__vmr_exec(region) || __vmr_dontexpand(region))
        return -EINVAL;
    
    if (!__vmr_read(region) && !__vmr_write(region))
        return -EINVAL;

    if (fb->fixinfo == NULL)
        return -EINVAL;

    if (__vmr_filepos(region) > fb->fixinfo->memsz)
        return -ERANGE; // Out of range not allowed.

    region->priv  = fb;
    region->vmops = &fbdev_vmrops;

    return 0;
}