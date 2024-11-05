#include <dev/dev.h>
#include <lib/string.h>
#include <sync/spinlock.h>
#include <lib/printk.h>
#include <bits/errno.h>

DEV_DECL_OPS(static, null);
static DEV_INIT(null, FS_CHR, DEV_NULL, 3);

static int null_init(void) {
    printk("Initializing \e[025453;011m%s\e[0m chardev...\n", nulldev.devname);
    return kdev_register(&nulldev, DEV_NULL, FS_CHR);
}

static int null_probe(void) {
    return 0;
}

static int null_close(struct devid *dd __unused) {
    return 0;
}

static int null_getinfo(struct devid *dd __unused, void *info __unused) {
    return -ENOTSUP;
}

static int null_open(struct devid *dd __unused, inode_t **pip __unused) {
    return 0;
}

static int null_ioctl(struct devid *dd __unused, int req __unused, void *argp __unused) {
    return -ENOTSUP;
}

static off_t null_lseek(struct devid *dd __unused, off_t off __unused, int whence __unused) {
    return -ENOTSUP;
}

static ssize_t null_read(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz __unused) {
    return 0;
}

static ssize_t null_write(struct devid *dd __unused, off_t off __unused, void *buf __unused, size_t sz) {
    return sz;
}

static int null_mmap(struct devid *dd, vmr_t *region) {
    if (dd == NULL || region == NULL)
        return -EINVAL;
    
    return -ENOSYS;
}

MODULE_INIT(null, NULL, null_init, NULL);