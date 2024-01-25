#include <dev/dev.h>
#include <sync/spinlock.h>
#include <lib/string.h>
#include <bits/errno.h>
#include <lib/printk.h>

static int full_init(void);
static int full_probe(void);
static int full_close(struct devid *dd);
static int full_getinfo(struct devid *dd, void *info);
static int full_open(struct devid *dd, int oflags, ...);
static int full_ioctl(struct devid *dd, int req, void *argp);
static off_t full_lseek(struct devid *dd, off_t off, int whence);
static ssize_t full_read(struct devid *dd, off_t off, void *buf, size_t sz);
static ssize_t full_write(struct devid *dd, off_t off, void *buf, size_t sz);
static int full_mmap(struct devid *dd, vmr_t *region);

static DEV_INIT(full, FS_CHR, DEV_FULL, 7);

static int full_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", fulldev.devname);
    return kdev_register(&fulldev, DEV_FULL, FS_CHR);
}

static int full_probe(void) {
    return 0;
}

static int full_close(struct devid *dd __unused) {
    return 0;
}

static int full_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int full_open(struct devid *dd __unused, int oflags __unused, ...) {
    return 0;
}

static int full_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return 0;
}

static off_t full_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return 0;
}

static ssize_t full_read(struct devid *dd __unused, off_t off __unused, void *buf, size_t sz) {
    if (buf == NULL)
        return -EINVAL;
    memset(buf, 0, sz);
    return sz;
}

static ssize_t full_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return -ENOSPC;
}

static int full_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    
    return -ENOSYS;
}

MODULE_INIT(full, NULL, full_init, NULL);